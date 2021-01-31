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

#include "Producer.hpp"
#include "Utils.hpp"

namespace HareCpp {

int Producer::QueueSize() const {
  std::lock_guard<std::mutex> lock{m_producerMutex};
  return m_sendQueue.size();
}

HARE_ERROR_E Producer::Send(const std::string& exchange,
                            const std::string& routingKey, Message& message) {
  auto retCode = HARE_ERROR_E::ALL_GOOD;

  if (addExchange(exchange) < 0)
    retCode = HARE_ERROR_E::INVALID_PARAMETERS;
  else {
    const std::lock_guard<std::mutex> lock{m_producerMutex};

    auto builtMessage = std::make_shared<helper::RawMessage>();

    builtMessage->exchange = hare_cstring_bytes(exchange.c_str());

    builtMessage->routing_key = hare_cstring_bytes(routingKey.c_str());

    builtMessage->properties = *message.AmqpProperties();

    builtMessage->message = amqp_bytes_malloc_dup(*message.Bytes());

    builtMessage->channel = m_exchangeList[exchange].m_channel;

    m_sendQueue.push(builtMessage);
  }

  return retCode;
}

HARE_ERROR_E Producer::Start() {
  auto retCode = HARE_ERROR_E::ALL_GOOD;
  LOG(LOG_DETAILED, "Producer thread Startup");

  if (false == IsInitialized()) {
    LOG(LOG_FATAL, "Producer not Initialized");
    retCode = HARE_ERROR_E::NOT_INITIALIZED;
  } else if (IsRunning()) {
    LOG(LOG_ERROR, "Thread already running");
    retCode = HARE_ERROR_E::THREAD_ALREADY_RUNNING;
  } else {
    setRunning(true);
    const std::lock_guard<std::mutex> lock(m_producerMutex);
    m_producerThread = std::thread(&Producer::thread, this);
    LOG(LOG_INFO, "Producer Thread Started");
  }

  return retCode;
}

HARE_ERROR_E Producer::Stop() {
  auto retCode = HARE_ERROR_E::ALL_GOOD;

  if (false == IsInitialized()) {
    LOG(LOG_ERROR, "Producer not initialized");
    retCode = HARE_ERROR_E::NOT_INITIALIZED;
  } else if (false == IsRunning()) {
    LOG(LOG_ERROR, "Producer thread not running");
    retCode = HARE_ERROR_E::THREAD_NOT_RUNNING;
  } else {
    setRunning(false);
    LOG(LOG_WARN, "Producer thread stopping");
    m_producerThread.join();
    m_channelsConnected = false;  // Needs to reconnect
  }

  m_connection->CloseConnection();
  for (std::pair<std::string, ExchangeProperties> element : m_exchangeList) {
    element.second.m_connected = false;
  }

  clearActiveSendQueue();

  return retCode;
}

/**
 * Intialize function
 */
HARE_ERROR_E Producer::Initialize(const std::string& server, int port,
                                  const std::string& username,
                                  const std::string& password) {
  auto retCode = HARE_ERROR_E::ALL_GOOD;

  {
    const std::lock_guard<std::mutex> lock{m_producerMutex};
    m_connection = std::make_shared<connection::ConnectionBase>(
        server, port, username, password);
  }

  if (noError(retCode)) {
    setInitialized(true);
    setRunning(false);
    LOG(LOG_INFO, "Producer Initialized Successfully")
  }

  return retCode;
}

Producer::~Producer() {
  if (IsInitialized()) {
    if (IsRunning()) Stop();
    // Close each channel, not entirely necessary with the CloseConnection()
    // call, however its safe.
    for (std::pair<std::string, ExchangeProperties> element : m_exchangeList) {
      m_connection->CloseChannel(element.second.m_channel);
      element.second.m_connected = false;
    }
    m_connection->CloseConnection();
  }

  LOG(LOG_INFO, "Producer deconstructed");
}

HARE_ERROR_E Producer::DeclareExchange(const std::string& exchange,
                                       const std::string& type) {
  auto channel = addExchange(exchange, type);
  return (channel != -1 ? HARE_ERROR_E::ALL_GOOD
                        : HARE_ERROR_E::INVALID_PARAMETERS);
}

HARE_ERROR_E Producer::Restart() {
  auto retCode = HARE_ERROR_E::ALL_GOOD;
  if (false == IsInitialized()) retCode = HARE_ERROR_E::NOT_INITIALIZED;
  if (noError(retCode)) retCode = Stop();
  if (noError(retCode)) retCode = Start();
  return retCode;
}

bool Producer::IsRunning() const {
  const std::lock_guard<std::mutex> lock{m_producerMutex};
  return m_threadRunning;
}

bool Producer::IsInitialized() const {
  const std::lock_guard<std::mutex> lock{m_producerMutex};
  return m_isInitialized;
}

}  // Namespace HareCpp