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

void Consumer::privateRestart() {
  // We do this because it is coming from the thread we are destroying.
  // Don't want to deadlock it/ Have 2 identical threads living, even if for
  // brief
  std::thread(&Consumer::Restart, this).detach();
}

// TODO this function needs cleaning
int Consumer::smartBind(int channel) {
  // If we are not connected, then we need to be trying to reconnect
  // and then periodically trying the channels again
  if (false == m_connection->IsConnected()) {
    m_unboundChannels.push(channel);
    return -1;
  }

  /**
   * Open the channel to be used to consume on
   */
  auto retCode = m_connection->OpenChannel(channel);
  if (noError(retCode)) {
    char log[80];
    sprintf(log, "Successfully opened channel: %d", channel);
    LOG(LOG_INFO, log);
  } else {
    // If there was a fatal exception (connection closed?) we need to restart
    // everything
    if (serverFailure(retCode)) {
      return -2;
    }
    char log[80];
    sprintf(log, "Unable to open channel: %d", channel);
    LOG(LOG_ERROR, log);
    m_unboundChannels.push(channel);
    return -1;
  }

  /**
   * Declare the queue so the broker can publish to it (given the
   * exchange/routing key combo)
   */
  amqp_bytes_t queueName;
  retCode = m_connection->DeclareQueue(
      channel, m_channelHandler.GetQueueProperties(channel), queueName);
  if (noError(retCode)) {
    char log[80];
    sprintf(log, "Created Queue: %s", hare_bytes_to_string(queueName).c_str());
    LOG(LOG_INFO, log);

    m_channelHandler.SetQueueName(channel, queueName);

    sprintf(log, "Binding: %s %s %s %d",
            hare_bytes_to_string(queueName).c_str(),
            m_channelHandler.GetExchange(channel).c_str(),
            m_channelHandler.GetBindingKey(channel).c_str(), channel);
    LOG(LOG_DETAILED, log);

  } else if (serverFailure(retCode)) {
    return -2;  // Need to restart
  } else {
    m_unboundChannels.push(channel);
    return -1;
  }

  /**
   * Bind to the created queue
   */
  {
    retCode = m_connection->BindQueue(channel, queueName,
                                      m_channelHandler.GetExchange(channel),
                                      m_channelHandler.GetBindingKey(channel));
    if (false == noError(retCode)) {
      if (serverFailure(retCode)) {
        return -2;
      }
      m_connection->CloseChannel(channel);
      m_unboundChannels.push(channel);
      return -1;
    }
  }

  /**
   *  Start consumption on the generated queue
   */
  retCode = m_connection->StartConsumption(channel, queueName);
  if (false == noError(retCode)) {
    if (serverFailure(retCode)) {
      return -2;
    } else {
      m_unboundChannels.push(channel);
      return -1;
    }
  }

  /**
   * Return the created channel number as proof of its creation and useage
   */
  return channel;
}

void Consumer::consume() {
  const std::lock_guard<std::mutex> lock(m_consumerMutex);
  // Empty unBoundChannels in case there are some stale ones
  while (false == m_unboundChannels.empty()) {
    m_unboundChannels.pop();
  }
  // Start up everything
  for (int channel : m_channelHandler.GetChannelList()) {
    {
      char log[80];
      sprintf(log, "Registering channel: %d", channel);
      LOG(LOG_DETAILED, log);
    }

    if (smartBind(channel) == -2) {
      privateRestart();
    }
  }
  if (m_unboundChannels.size() != 0) {
    m_unboundChannelThreadSig = new std::promise<void>();
    m_futureObjUnboundChannel = m_unboundChannelThreadSig->get_future();
    LOG(LOG_WARN, "Unbound Channel Thread Starting");

    m_unboundChannelThreadRunning = true;
    m_unboundChannelThread = std::thread(&Consumer::unboundChannelThread, this);

    LOG(LOG_WARN, "Unbound Channel Thread Started");
    // TODO Start unboundChannelThread
  } else {
    LOG(LOG_INFO, "All Consumer Channels successfully created");
  }
}

void Consumer::thread() {
  while (m_futureObj.wait_for(std::chrono::milliseconds(0)) ==
         std::future_status::timeout) {
    if ((false == m_connection->IsConnected()) && m_threadRunning) {
      auto retCode = m_connection->Connect();
      if (noError(retCode)) {
        consume();
        // Start back at the begining before starting to consume
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

    const std::lock_guard<std::mutex> lock(m_consumerMutex);

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
        char log[80];
        sprintf(log, "Received message on %s : %s", exchange.c_str(),
                bindingKey.c_str());
        LOG(LOG_DETAILED, log);
      }

      m_channelHandler.Process(std::make_pair(exchange, bindingKey),
                               newMessage);

    } else if (serverFailure(ret)) {
      LOG(LOG_FATAL, "Restarting Consumer due to server error");
      privateRestart();
    }

    amqp_destroy_envelope(&envelope);
  }
}

void Consumer::unboundChannelThread() {
  while (m_futureObjUnboundChannel.wait_for(std::chrono::milliseconds(200)) ==
         std::future_status::timeout) {
    if (false == m_connection->IsConnected()) {
      continue;
    }
    // TODO LOCK!!!!!!!!!!!!!!!! but figure out a smart way to do it
    // Maybe the connection lock down a few lines is sufficient
    if (m_unboundChannels.size() == 0) {
      break;
    }
    int channel = m_unboundChannels.front();
    m_unboundChannels.pop();

    LOG(LOG_DETAILED, "Trying a channel");
    if (false == m_unboundChannelThreadRunning) {
      break;
    }

    if (smartBind(channel) == -2) {
      privateRestart();
    }
  }
  LOG(LOG_INFO, "UnboundChannelThread Finished");
}
}  // Namespace HareCpp