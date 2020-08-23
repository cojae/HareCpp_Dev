/*
 * libharecpp - Wrapper Library around: rabbitmq-c - rabbitmq C library
 *
 * Copyright (c) 2020 Cody Williams
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdio.h>
#include <stdlib.h>
#include <cstring>

#include "Consumer.hpp"
#include "Utils.hpp"

namespace HareCpp {

void Consumer::stopUnboundChannelThread() {
  if (m_unboundChannelThreadRunning) {
    LOG(LOG_WARN, "Unbound Channel Thread Stopping");
    m_unboundChannelThreadRunning = false;

    m_unboundChannelThreadSig->set_value();
    m_unboundChannelThread.join();

    delete m_unboundChannelThreadSig;
  }
}

HARE_ERROR_E Consumer::openChannel(const int channel) {
  auto retCode = m_connection->OpenChannel(channel);
  if (noError(retCode)) {
    char log[LOG_MAX_CHAR_SIZE];
    snprintf(log,LOG_MAX_CHAR_SIZE,"Successfully opened channel: %d", channel);
    LOG(LOG_INFO, log);
  } else {
    char log[LOG_MAX_CHAR_SIZE];
    snprintf(log,LOG_MAX_CHAR_SIZE,"Unable to open channel: %d", channel);
    LOG(LOG_ERROR, log);
  }
  return retCode;
}

HARE_ERROR_E Consumer::declareQueue(const int channel,
                                    amqp_bytes_t& queueName) {
  auto retCode = m_connection->DeclareQueue(
      channel, m_channelHandler.GetQueueProperties(channel), queueName);
  if (noError(retCode)) {
    char log[LOG_MAX_CHAR_SIZE];
    snprintf(log,LOG_MAX_CHAR_SIZE,"Created Queue: %s", hare_bytes_to_string(queueName).c_str());
    LOG(LOG_INFO, log);

    m_channelHandler.SetQueueName(channel, queueName);
  }
  return retCode;
}

HARE_ERROR_E Consumer::bindQueue(const int channel,
                                 const amqp_bytes_t& queueName) {
  char log[LOG_MAX_CHAR_SIZE];
  snprintf(log,LOG_MAX_CHAR_SIZE,"Binding: %s %s %s %d", hare_bytes_to_string(queueName).c_str(),
          m_channelHandler.GetExchange(channel).c_str(),
          m_channelHandler.GetBindingKey(channel).c_str(), channel);
  LOG(LOG_DETAILED, log);

  auto retCode = m_connection->BindQueue(
      channel, queueName, m_channelHandler.GetExchange(channel),
      m_channelHandler.GetBindingKey(channel));
  return retCode;
}

HARE_ERROR_E Consumer::consume(const int channel,
                               const amqp_bytes_t& queueName) {
  auto retCode = m_connection->StartConsumption(channel, queueName);
  return retCode;
}

HARE_ERROR_E Consumer::setupAndConsume(int channel) {
  auto retCode = HARE_ERROR_E::ALL_GOOD;
  amqp_bytes_t queueName;

  char log[LOG_MAX_CHAR_SIZE];
  snprintf(log,LOG_MAX_CHAR_SIZE,"Registering channel: %d", channel);
  LOG(LOG_DETAILED, log);

  if (false == m_connection->IsConnected()) {
    retCode = HARE_ERROR_E::SERVER_CONNECTION_FAILURE;
  }

  if (noError(retCode)) retCode = openChannel(channel);
  if (noError(retCode)) retCode = declareQueue(channel, queueName);
  if (noError(retCode)) retCode = bindQueue(channel, queueName);
  if (noError(retCode)) retCode = consume(channel, queueName);

  if (false == noError(retCode)) {
    m_unboundChannels.push(channel);
  }

  return retCode;
}

HARE_ERROR_E Consumer::startConsumption() {
  const std::lock_guard<std::mutex> lock(m_consumerMutex);
  auto retCode = HARE_ERROR_E::ALL_GOOD;

  // Empty unBoundChannels in case there are some stale ones
  while (false == m_unboundChannels.empty()) {
    m_unboundChannels.pop();
  }

  // Start up everything
  for (int channel : m_channelHandler.GetChannelList()) {
    retCode = setupAndConsume(channel);
    if (serverFailure(retCode)) {
      return retCode;
    }
  }
  if (m_unboundChannels.size() != 0) {
    startUnboundChannelThread();
  } else {
    LOG(LOG_INFO, "All Consumer Channels successfully created");
  }
  return retCode;
}

void Consumer::startUnboundChannelThread() {
  m_unboundChannelThreadSig = new std::promise<void>();
  m_futureObjUnboundChannel = m_unboundChannelThreadSig->get_future();
  LOG(LOG_WARN, "Unbound Channel Thread Starting");

  m_unboundChannelThreadRunning = true;
  m_unboundChannelThread = std::thread(&Consumer::unboundChannelThread, this);

  LOG(LOG_WARN, "Unbound Channel Thread Started");
}

void Consumer::thread() {
  while (m_futureObj.wait_for(std::chrono::milliseconds(0)) ==
         std::future_status::timeout) {
    if ((false == m_connection->IsConnected()) && m_threadRunning) {
      auto retCode = m_connection->Connect();
      if (noError(retCode)) {
        retCode = startConsumption();
        if (serverFailure(retCode)) {
          if (m_unboundChannelThreadRunning) {
            stopUnboundChannelThread();
          }

          m_connection->CloseConnection();
          return;
        }
        continue;
      } else {
        // Sleep a configurable (TODO) amount of time to reduce spamming a
        // restarted broker. This does actually speed up the time to reconnect
        // by having a sleep
        std::this_thread::sleep_for(
            std::chrono::milliseconds(CONNECTION_RETRY_TIMEOUT_MILLISECONDS));
        continue;
      }
    }

    if (m_connection->IsConnected())
      amqp_maybe_release_buffers(m_connection->Connection());

    amqp_envelope_t envelope;

    auto ret = m_connection->ConsumeMessage(envelope);

    if (noError(ret)) {
      Message newMessage(envelope);

      // envelope was received but malformed
      if (envelope.exchange.len == 0) {
        amqp_destroy_envelope(&envelope);
        continue;
      }

      auto exchange = std::string(static_cast<char*>(envelope.exchange.bytes),
                                  envelope.exchange.len);

      auto bindingKey =
          std::string(static_cast<char*>(envelope.routing_key.bytes),
                      envelope.routing_key.len);

      {
        char log[LOG_MAX_CHAR_SIZE];
        snprintf(log,LOG_MAX_CHAR_SIZE,"Received message on %s : %s", exchange.c_str(),
                bindingKey.c_str());
        LOG(LOG_DETAILED, log);
      }

      m_channelHandler.Process(std::make_pair(exchange, bindingKey),
                               newMessage);

      amqp_destroy_envelope(&envelope);

    } else if (serverFailure(ret) && m_threadRunning) {
      LOG(LOG_FATAL, "Restarting Consumer due to server error");
      if (m_unboundChannelThreadRunning) {
        stopUnboundChannelThread();
      }

      m_connection->CloseConnection();
      return;
    }
  }
}

void Consumer::unboundChannelThread() {
  while (m_futureObjUnboundChannel.wait_for(std::chrono::milliseconds(200)) ==
         std::future_status::timeout) {
    if (false == m_connection->IsConnected()) {
      continue;
    }
    if (m_unboundChannels.size() == 0) {
      break;
    }
    int channel = m_unboundChannels.front();
    m_unboundChannels.pop();

    LOG(LOG_DETAILED, "Trying a channel");
    if (false == m_unboundChannelThreadRunning) {
      break;
    }

    auto retCode = setupAndConsume(channel);
    if (serverFailure(retCode)) {
      return;
    }
  }
  LOG(LOG_INFO, "UnboundChannelThread Finished");
}
}  // Namespace HareCpp