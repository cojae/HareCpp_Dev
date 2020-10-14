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

HARE_ERROR_E Producer::Send(const std::string& routing_value,
                            Message& message) {
  /**
   *    Don't use scope lock as this may call the other send function
   *  And that one will get blocked if this holds the mutex
   */
  m_producerMutex.lock();
  auto retCode = HARE_ERROR_E::ALL_GOOD;

  if (false == m_isInitialized) {
    LOG(LOG_FATAL, "Producer not initialized");
    retCode = HARE_ERROR_E::NOT_INITIALIZED;
  }

  if (noError(retCode) && m_exchange == "") {
    LOG(LOG_ERROR, "No exchange set, cannot send\n");
    retCode = HARE_ERROR_E::INVALID_PARAMETERS;
  }

  std::string exchangeToUse = m_exchange;
  m_producerMutex.unlock();

  if (noError(retCode)) {
    retCode = Send(exchangeToUse, routing_value, message);
  }

  return retCode;
}

int Producer::QueueSize() const {
  std::lock_guard<std::mutex> lock{m_producerMutex};
  return m_sendQueue.size();
}

HARE_ERROR_E Producer::Send(const std::string& exchange,
                            const std::string& routing_value,
                            Message& message) {
  auto retCode = HARE_ERROR_E::ALL_GOOD;

  if (addExchange(exchange) < 0) 
    retCode = HARE_ERROR_E::INVALID_PARAMETERS ;
  else {

    const std::lock_guard<std::mutex> lock{m_producerMutex};

    auto builtMessage = std::make_shared<helper::RawMessage>();

    builtMessage->exchange = hare_cstring_bytes(exchange.c_str());

    builtMessage->routing_value = hare_cstring_bytes(routing_value.c_str());

    builtMessage->properties = *message.getAmqpProperties();

    builtMessage->message = hare_cstring_bytes(message.payload()->c_str());

    builtMessage->channel = m_exchangeList[m_exchange].m_channel;

    m_sendQueue.push(builtMessage);
  }

  return retCode;
}

HARE_ERROR_E Producer::Start() {
  const std::lock_guard<std::mutex> lock{m_producerMutex};

  m_exitThreadSignal = new std::promise<void>();

  auto retCode = HARE_ERROR_E::ALL_GOOD;
  LOG(LOG_DETAILED, "Producer thread Startup");

  if (false == m_isInitialized) {
    LOG(LOG_FATAL, "Producer not Initialized");
    retCode = HARE_ERROR_E::NOT_INITIALIZED;
  }

  if (noError(retCode) && m_threadRunning) {
    LOG(LOG_ERROR, "Thread already running");
    retCode = HARE_ERROR_E::THREAD_ALREADY_RUNNING;
  }

  if (noError(retCode)) {
    m_futureObj = m_exitThreadSignal->get_future();

    m_producerThread = std::thread(&Producer::thread, this);

    LOG(LOG_INFO, "Producer Thread Started");

    m_threadRunning = true;
  }
  return retCode;
}

HARE_ERROR_E Producer::Stop() {
  auto retCode = HARE_ERROR_E::ALL_GOOD;

  if (false == m_isInitialized) {
    LOG(LOG_ERROR, "Producer not initialized");
    retCode = HARE_ERROR_E::NOT_INITIALIZED;
  }

  if (noError(retCode) && false == m_threadRunning) {
    LOG(LOG_ERROR, "Producer thread not running");
    retCode = HARE_ERROR_E::THREAD_NOT_RUNNING;
  } else if (noError(retCode)) {
    m_threadRunning = false;
    LOG(LOG_WARN, "Producer thread stopping");
    m_exitThreadSignal->set_value();
    m_producerThread.join();

    delete m_exitThreadSignal;
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
  // Set up connection information
  m_producerMutex.lock();
  auto retCode = HARE_ERROR_E::ALL_GOOD;
  m_connection = std::make_shared<connection::ConnectionBase>(
      server, port, username, password);

  if (noError(retCode)) {
    m_isInitialized = true;
    m_threadRunning = false;
    LOG(LOG_INFO, "Producer Initialized Successfully")
  }

  m_producerMutex.unlock();

  if (m_exchange != "") {
    addExchange(m_exchange);
    LOG(LOG_DETAILED, "First Default Exchange Set");
  }
  return retCode;
}

Producer::~Producer() {
  if (IsInitialized()) {
    if (IsRunning()) {
      Stop();
    }

    for (std::pair<std::string, ExchangeProperties> element : m_exchangeList) {
      m_connection->CloseChannel(element.second.m_channel);
      element.second.m_connected = false;
    }

    m_connection->CloseConnection();
  }

  LOG(LOG_INFO, "Producer deconstructed");
}


void Producer::SetExchange(const std::string& exchange) {
  addExchange(exchange);
}

HARE_ERROR_E Producer::DeclareExchange(const std::string& exchange,
                                       const std::string& type) {
  auto retCode = HARE_ERROR_E::ALL_GOOD;
  if (noError(retCode)) {
    auto channel = addExchange(exchange, type);
    if (channel == -1) {
      retCode = HARE_ERROR_E::INVALID_PARAMETERS;
    }
  }
  return retCode;
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