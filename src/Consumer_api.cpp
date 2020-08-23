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
#include "Consumer.hpp"
#include "Utils.hpp"

namespace HareCpp {

HARE_ERROR_E Consumer::Subscribe(const std::string& exchange,
                                 const std::string& binding_key, TD_Callback f,
                                 helper::queueProperties queueProps) {
  std::lock_guard<std::mutex> lock(m_consumerMutex);
  auto retCode = HARE_ERROR_E::ALL_GOOD;

  if (false == m_isInitialized) {
    LOG(LOG_FATAL, "Consumer Not Initialized");
    retCode = HARE_ERROR_E::NOT_INITIALIZED;
  }

  if (m_threadRunning) {
    LOG(LOG_ERROR, "Thread Already Running, cannot subscribe TODO");
    retCode = HARE_ERROR_E::THREAD_ALREADY_RUNNING;
  }

  if (noError(retCode)) {
    auto channel = m_channelHandler.AddChannelProcessor(
        std::make_pair(exchange, binding_key), f);

    if (channel == -1) {
      char log[LOG_MAX_CHAR_SIZE];
      snprintf(log,LOG_MAX_CHAR_SIZE,"Unable to subscribe to %s : %s", exchange.c_str(),
              binding_key.c_str());
      LOG(LOG_ERROR, log);
      retCode = HARE_ERROR_E::UNABLE_TO_SUBSCRIBE;
    } else {
    }
    if (noError(retCode)) {
      m_channelHandler.SetQueueProperties(channel, queueProps);
    }
  }
  return retCode;
}

HARE_ERROR_E Consumer::Start() {
  const std::lock_guard<std::mutex> threadLock(m_threadMutex);
  std::lock_guard<std::mutex> lock(m_consumerMutex);
  auto retCode = HARE_ERROR_E::ALL_GOOD;

  LOG(LOG_DETAILED, "Consumer Thread Startup");

  if (false == m_isInitialized) {
    LOG(LOG_ERROR, "Consumer not Initialized");
    retCode = HARE_ERROR_E::NOT_INITIALIZED;
  }

  if (noError(retCode) && m_threadRunning) {
    LOG(LOG_WARN, "Thread already running");
    retCode = HARE_ERROR_E::THREAD_ALREADY_RUNNING;
  }

  if (noError(retCode)) {
    m_exitThreadSignal = new std::promise<void>();

    m_futureObj = m_exitThreadSignal->get_future();

    m_threadRunning = true;

    // Start up the consumer thread
    m_consumerThread = std::thread(&Consumer::thread, this);

    LOG(LOG_INFO, "Consumer Thread Started");
  }

  return retCode;
}

HARE_ERROR_E Consumer::Stop() {
  const std::lock_guard<std::mutex> threadLock(m_threadMutex);
  m_consumerMutex.lock();

  auto retCode = HARE_ERROR_E::ALL_GOOD;

  if (false == m_isInitialized) {
    LOG(LOG_ERROR, "Consumer not initialized");
    retCode = HARE_ERROR_E::NOT_INITIALIZED;
  }

  if (noError(retCode) && false == m_threadRunning) {
    retCode = HARE_ERROR_E::THREAD_NOT_RUNNING;
  } else if (noError(retCode)) {
    m_threadRunning = false;

    LOG(LOG_WARN, "Consumer thread stopping");

    m_exitThreadSignal->set_value();
    m_consumerMutex.unlock();  // So we can kill off the thread
    m_consumerThread.join();

    delete m_exitThreadSignal;

    if (m_unboundChannelThreadRunning) {
      stopUnboundChannelThread();
    }

    m_consumerMutex.lock();  // Regrab the lock, just in case

    retCode = m_connection->CloseConnection();
  }

  m_consumerMutex.unlock();

  return retCode;
}

/**
 * Intialize function
 */
HARE_ERROR_E Consumer::Initialize(const std::string& server, int port,
                                  const std::string& username,
                                  const std::string& password) {
  auto retCode = HARE_ERROR_E::ALL_GOOD;
  m_connection = std::make_shared<connection::ConnectionBase>(
      server, port, username, password);

  if (noError(retCode)) {
    m_isInitialized = true;
    m_threadRunning = false;
    m_unboundChannelThreadRunning = false;
    LOG(LOG_INFO, "Consumer Initialized Successfully")
  }
  return retCode;
}

HARE_ERROR_E Consumer::Restart() {
  auto retCode = HARE_ERROR_E::ALL_GOOD;
  retCode = Stop();
  if (noError(retCode)) {
    retCode = Start();
  }
  return retCode;
}

}  // namespace HareCpp