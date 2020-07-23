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

HARE_ERROR_E Producer::send(const std::string& routing_value,
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
    retCode = send(exchangeToUse, routing_value, message);
  }

  return retCode;
}

HARE_ERROR_E Producer::send(const std::string& exchange,
                            const std::string& routing_value,
                            Message& message) {
  auto retCode = HARE_ERROR_E::ALL_GOOD;

  // TODO CHeck return value
  addExchange(exchange);

  // TODO set the timestamp either HERE or when its gets SENT
  // Both have pros and cons
  // NOTE: i did it in the send (connectionBase)
  // Might be good to keep it there, and if the user wants to set it theirselves they can

  const std::lock_guard<std::mutex> lock(m_producerMutex);

  auto builtMessage = std::make_shared<helper::RawMessage>();

  builtMessage->exchange = hare_cstring_bytes(exchange.c_str());

  builtMessage->routing_value = hare_cstring_bytes(routing_value.c_str());

  builtMessage->properties = *message.getAmqpProperties();

  builtMessage->message = hare_cstring_bytes(message.payload()->c_str());

  builtMessage->channel = m_exchangeList[m_exchange].m_channel;

  // TODO allow user to change this default value...also make this constexpr
  // OR don't even do this ?
  /*if ( m_sendQueue.size() >= 15 ) {
    retCode = HARE_ERROR_E::PRODUCER_QUEUE_FULL;
    m_sendQueue.pop(); // Pop the next in line off
  }*/  
  // NOTE, this breaks shit... why? idk plz investigate. may show me a race condition TODO

  m_sendQueue.push(builtMessage);

  /*
    {
      char log[80];
      sprintf(log, "Queueing up message on exchange: %s", m_exchange.c_str());
      LOG(LOG_DETAILED, log);
    }
    */
  return retCode;  // TODO addExchange should return error code
}

HARE_ERROR_E Producer::start() {
  const std::lock_guard<std::mutex> lock(m_producerMutex);

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

void Producer::thread() {
  while (m_futureObj.wait_for(std::chrono::milliseconds(0)) ==
         std::future_status::timeout) {
    const std::lock_guard<std::mutex> lock(m_producerMutex);

    if(false == m_threadRunning) {
      return;
    }

    if(false == m_connection->IsConnected()) {
      auto retCode = m_connection->Connect();
      if(false == noError(retCode)) {
        // Sleep a TODO configurable amount of time
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        continue;
      }
    }

    // Declare exchanges if not been declared
    // TODO possibly spawn off a seperate thread for this?
    if (false == m_channelsConnected) {
      bool allGood {true};
      if (false == m_threadRunning) continue;
      for (auto it : m_exchangeList) {
        if (false == it.second.m_connected) {
          auto retCode = m_connection->OpenChannel(it.second.m_channel);
          if(serverFailure(retCode)) {
            privateRestart();
          }

          if (false == noError(retCode)) {
            //if(serverFailure(retCode)) { Restart(); }
            allGood = false;
            continue;
          }
          if (it.second.m_isDeclare) {
            retCode = m_connection->DeclareChannel(it.second.m_channel,
                                                   it.first, it.second.m_type);
            // TODO IF Error, reset channel (maybe remove from exchange list)
            if(serverFailure(retCode)) {
              privateRestart();
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
    }

    if (m_sendQueue.size() == 0) {
      continue;
    }
    auto retCode = m_connection->PublishMessage(*m_sendQueue.front());
    if(serverFailure(retCode)) {
      privateRestart();
    }

    // TODO if we restarted AND/OR if we are max out of the queue, 
    // make sure we are freeing these values!!!!!!!!!!!!
    if (noError(retCode)) {
      amqp_bytes_free(m_sendQueue.front()->message);
      amqp_bytes_free(m_sendQueue.front()->routing_value);
      amqp_bytes_free(m_sendQueue.front()->exchange);
      m_sendQueue.pop();
    }
  }
}

HARE_ERROR_E Producer::stop() {
  // const std::lock_guard<std::mutex> lock(m_producerMutex);
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
  while(false == m_sendQueue.empty()) {
    amqp_bytes_free(m_sendQueue.front()->message);
    amqp_bytes_free(m_sendQueue.front()->routing_value);
    amqp_bytes_free(m_sendQueue.front()->exchange);
    m_sendQueue.pop();
  }

  return retCode;
}

/**
 * Intialize function
 */
HARE_ERROR_E Producer::initialize(const std::string& server, int port) {
  // Set up connection information
  m_producerMutex.lock();
  auto retCode = HARE_ERROR_E::ALL_GOOD;
  m_connection = std::make_shared<connection::ConnectionBase>(server, port);
  //retCode = m_connection->Connect();

  if (noError(retCode)) {
    m_isInitialized = true;
    m_threadRunning = false;
    LOG(LOG_INFO, "Producer Initialized Successfully")
  }

  m_producerMutex.unlock();

  // If exchange is set (probably only using the one)
  // Open channel
  if (m_exchange != "") {
    addExchange(m_exchange);
    LOG(LOG_DETAILED, "First Default Exchange Set");
  }
  return retCode;
}

Producer::~Producer() {
  if (m_threadRunning) {
    stop();
  }

  for (std::pair<std::string, ExchangeProperties> element : m_exchangeList) {
    m_connection->CloseChannel(element.second.m_channel);
    element.second.m_connected = false;
  }

  m_connection->CloseConnection();

  LOG(LOG_INFO, "Producer deconstructed");
}

int Producer::addExchange(const std::string& exchange,
                          const std::string& type) {
  const std::lock_guard<std::mutex> lock(m_producerMutex);

  m_exchange = exchange;

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

  // Default is to use the last used/created exchange
  m_exchange = exchange;

  int selectedChannel = -1;

  if (m_exchangeList.find(exchange) == m_exchangeList.end()) {
    m_exchangeList[exchange] = ExchangeProperties();
    m_exchangeList[exchange].m_channel = m_curChannelNumber;
    m_channelsConnected = false;
    m_curChannelNumber++;
  } else {
    selectedChannel = m_exchangeList[exchange].m_channel;
  }
  return selectedChannel;
}

void Producer::setExchange(const std::string& exchange) {
  addExchange(exchange);
}

HARE_ERROR_E Producer::declareExchange(const std::string& exchange,
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
  retCode = stop();
  if(noError(retCode)) {
    retCode = start();
  }
  return retCode;
}

void Producer::privateRestart() {
  std::thread(&Producer::Restart, this).detach();
}

}  // Namespace HareCpp