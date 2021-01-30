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

void Producer::thread() {
  while (IsRunning()) {
    if (false == isConnected()) {
      auto retCode = m_connection->Connect();
      if (false == noError(retCode)) {
        // Sleep a TODO configurable amount of time
        // This sleep is important to not spam the rabbitmq broker
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        continue;
      }
    }

    // Declare exchanges if not been declared
    if (false == channelsConnected()) {
      connectChannels();
    }

    publishNextInQueue();
  }
}

void Producer::setRunning(bool running) {
  const std::lock_guard<std::mutex> lock(m_producerMutex);
  m_threadRunning = running;
}

int Producer::addExchange(const std::string& exchange,
                          const std::string& type) {
  const std::lock_guard<std::mutex> lock(m_producerMutex);

  int selectedChannel = -1;

  if (m_exchangeList.find(exchange) == m_exchangeList.end()) {
    m_exchangeList[exchange] =
        ExchangeProperties(m_curChannelNumber, true, type);
    m_channelsConnected = false;
    selectedChannel = m_curChannelNumber;
    m_curChannelNumber++;
  } else {
    // hopefully this is never hit...
    // things could get weird if you declare after using it
    LOG(LOG_WARN, "Declaring an exchange after already setting/using it");
    m_exchangeList[exchange].m_isDeclare = true;
    m_exchangeList[exchange].m_type = type;
    selectedChannel = m_exchangeList[exchange].m_channel;
    m_channelsConnected = false;
  }
  return selectedChannel;
}

int Producer::addExchange(const std::string& exchange) {
  const std::lock_guard<std::mutex> lock(m_producerMutex);

  int selectedChannel = -1;

  if (m_exchangeList.find(exchange) == m_exchangeList.end()) {
    m_exchangeList[exchange] = ExchangeProperties();
    m_exchangeList[exchange].m_channel = m_curChannelNumber;
    m_channelsConnected = false;
    selectedChannel = m_curChannelNumber;
    m_curChannelNumber++;
  } else {
    selectedChannel = m_exchangeList[exchange].m_channel;
  }
  return selectedChannel;
}

void Producer::clearActiveSendQueue() {
  while (false == m_sendQueue.empty()) {
    hare_free_message_risky(*m_sendQueue.front());
    m_sendQueue.pop();
  }
}

bool Producer::channelsConnected() const {
  const std::lock_guard<std::mutex> lock(m_producerMutex);
  return m_channelsConnected;
}

void Producer::closeConnection() {
  m_connection->CloseConnection();
  for (auto element : m_exchangeList) element.second.m_connected = false;
  clearActiveSendQueue();
}

void Producer::connectChannels() {
  bool allGood{true};

  if (false == isConnected() || false == IsRunning()) return;

  m_producerMutex.lock();
  for (auto it : m_exchangeList) {
    if (false == it.second.m_connected) {
      auto retCode = m_connection->OpenChannel(it.second.m_channel);
      if (serverFailure(retCode)) {
        m_producerMutex.unlock();
        closeConnection();
        return;
      }

      if (false == noError(retCode)) {
        allGood = false;
        continue;
      }
      if (it.second.m_isDeclare) {
        retCode = m_connection->DeclareExchange(it.second.m_channel, it.first,
                                                it.second.m_type);
        if (serverFailure(retCode)) {
          m_producerMutex.unlock();
          closeConnection();
          return;
        }
      }
      if (noError(retCode)) {
        it.second.m_connected = true;
      }
    }
  }

  if (allGood) {
    m_channelsConnected = true;
  }

  m_producerMutex.unlock();
}

void Producer::publishNextInQueue() {
  if (false == isConnected()) return;

  if (m_sendQueue.size() != 0) {
    auto retCode = m_connection->PublishMessage(*m_sendQueue.front());
    if (serverFailure(retCode)) {
      closeConnection();
      return;
    }

    // If sent, pop it off the queue
    if (noError(retCode)) {
      const std::lock_guard<std::mutex> lock{m_producerMutex};
      hare_free_message_risky(*m_sendQueue.front());
      m_sendQueue.pop();
    }
  }
}

bool Producer::isConnected() const {
  const std::lock_guard<std::mutex> lock{m_producerMutex};
  return (m_connection == nullptr ? false : m_connection->IsConnected());
}

void Producer::setInitialized(bool initialized) {
  const std::lock_guard<std::mutex> lock{m_producerMutex};
  m_isInitialized = initialized;
}

}  // Namespace HareCpp