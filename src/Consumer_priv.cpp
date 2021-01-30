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

void Consumer::setRunning(bool running) {
  const std::lock_guard<std::mutex> lock(m_consumerMutex);
  m_threadRunning = running;
}

HARE_ERROR_E Consumer::openChannel(const int channel) {
  auto retCode = m_connection->OpenChannel(channel);
  if (noError(retCode)) {
    char log[LOG_MAX_CHAR_SIZE];
    snprintf(log, LOG_MAX_CHAR_SIZE, "Successfully opened channel: %d",
             channel);
    LOG(LOG_INFO, log);
  } else {
    char log[LOG_MAX_CHAR_SIZE];
    snprintf(log, LOG_MAX_CHAR_SIZE, "Unable to open channel: %d", channel);
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
    snprintf(log, LOG_MAX_CHAR_SIZE, "Created Queue: %s",
             hare_bytes_to_string(queueName).c_str());
    LOG(LOG_INFO, log);

    m_channelHandler.SetQueueName(channel, queueName);
  }
  return retCode;
}

HARE_ERROR_E Consumer::bindQueue(const int channel,
                                 const amqp_bytes_t& queueName) {
  char log[LOG_MAX_CHAR_SIZE];
  snprintf(log, LOG_MAX_CHAR_SIZE, "Binding: %s %s %s %d",
           hare_bytes_to_string(queueName).c_str(),
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

int Consumer::pendingChannelSize() const {
  const std::lock_guard<std::mutex> lock{m_pendingChannelMutex};
  return m_pendingChannels.size();
}

int Consumer::popNextPendingChannel() {
  const std::lock_guard<std::mutex> lock{m_pendingChannelMutex};
  int retChannel = m_pendingChannels.front();
  m_pendingChannels.pop();
  return retChannel;
}

void Consumer::pushIntoPendingChannels(const int channel) {
  const std::lock_guard<std::mutex> lock{m_pendingChannelMutex};
  m_pendingChannels.push(channel);
}

void Consumer::emptyPendingChannels() {
  const std::lock_guard<std::mutex> lock{m_pendingChannelMutex};
  while (false == m_pendingChannels.empty()) {
    m_pendingChannels.pop();
  }
}

HARE_ERROR_E Consumer::setupAndConsume(int channel) {
  auto retCode = HARE_ERROR_E::ALL_GOOD;
  amqp_bytes_t queueName;

  char log[LOG_MAX_CHAR_SIZE];
  snprintf(log, LOG_MAX_CHAR_SIZE, "Registering channel: %d", channel);
  LOG(LOG_DETAILED, log);

  if (noError(retCode)) retCode = openChannel(channel);
  if (noError(retCode)) retCode = declareQueue(channel, queueName);
  if (noError(retCode)) retCode = bindQueue(channel, queueName);
  if (noError(retCode)) retCode = consume(channel, queueName);

  if (false == noError(retCode)) {
    pushIntoPendingChannels(channel);
  }

  return retCode;
}

HARE_ERROR_E Consumer::startConsumption() {
  const std::lock_guard<std::mutex> lock(m_consumerMutex);
  auto retCode = HARE_ERROR_E::ALL_GOOD;

  // Empty pendingChannels in case there are some stale ones
  emptyPendingChannels();

  // Start up everything
  for (int channel : m_channelHandler.GetChannelList()) {
    retCode = setupAndConsume(channel);
    if (serverFailure(retCode)) {
      return retCode;
    }
  }
  // Start up thread to retry all connections that couldn't be established
  if (pendingChannelSize() == 0) {
    LOG(LOG_INFO, "All Consumer Channels successfully created");
  }
  return retCode;
}

void Consumer::thread() {
  HARE_ERROR_E retCode;
  while (IsRunning()) {
    retCode = HARE_ERROR_E::ALL_GOOD;
    if (false == m_connection->IsConnected()) {
      retCode = connectAndStartConsumption();
    }

    if (noError(retCode)) pullNextMessage();

    // Are there any subscriptions made that aren't connected to?
    // Pop one at a time so as to not halt up consumption of messages
    if (pendingChannelSize() != 0) {
      LOG(LOG_DETAILED, "Attempting to connect to a channel");
      retCode = setupAndConsume(popNextPendingChannel());
    }
  }
}

HARE_ERROR_E Consumer::connectAndStartConsumption() {
  auto retCode = m_connection->Connect();
  if (noError(retCode)) {
    retCode = startConsumption();
    if (serverFailure(retCode)) {
      m_connection->CloseConnection();
    }
  } else {
    // Sleep a configurable amount of time to reduce spamming a
    // restarted broker. This does actually speed up the time to reconnect
    // by having a sleep
    std::this_thread::sleep_for(
        std::chrono::milliseconds(CONNECTION_RETRY_TIMEOUT_MILLISECONDS));
  }
  return retCode;
}

void Consumer::pullNextMessage() {
  if (m_connection->IsConnected())
    amqp_maybe_release_buffers(m_connection->Connection());
  else
    return;

  amqp_envelope_t envelope;

  auto ret = m_connection->ConsumeMessage(envelope);

  if (noError(ret)) {
    Message newMessage(envelope);

    // envelope was received but malformed
    if (envelope.exchange.len == 0) {
      amqp_destroy_envelope(&envelope);
      return;
    }

    auto exchange = std::string(static_cast<char*>(envelope.exchange.bytes),
                                envelope.exchange.len);

    auto bindingKey =
        std::string(static_cast<char*>(envelope.routing_key.bytes),
                    envelope.routing_key.len);

    {
      char log[LOG_MAX_CHAR_SIZE];
      snprintf(log, LOG_MAX_CHAR_SIZE, "Received message on %s : %s",
               exchange.c_str(), bindingKey.c_str());
      LOG(LOG_DETAILED, log);
    }

    m_channelHandler.Process(std::make_pair(exchange, bindingKey), newMessage);

    amqp_destroy_envelope(&envelope);

  } else if (serverFailure(ret) && IsRunning()) {
    LOG(LOG_FATAL, "Restarting Consumer due to server error");
    m_connection->CloseConnection();
    return;
  }
}

}  // Namespace HareCpp