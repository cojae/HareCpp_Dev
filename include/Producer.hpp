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

#ifndef _PRODUCER_H_
#define _PRODUCER_H_

#include "ConnectionBase.hpp"
#include "Message.hpp"
#include "pch.hpp"

#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>

namespace HareCpp {

class Producer {
 private:
  struct ExchangeProperties {
    ExchangeProperties(int channel = -1, bool isDeclare = false,
                       const std::string& type = "direct")
        : m_channel(channel), m_isDeclare(isDeclare), m_type(type){};
    int m_channel;
    bool m_connected;

    bool m_isDeclare;  // Do we need to declare this exchange?
    std::string m_type;
  };

  void privateRestart();

  std::unordered_map<std::string, ExchangeProperties> m_exchangeList;

  std::shared_ptr<connection::ConnectionBase> m_connection;

  std::string m_exchange;

  bool m_isInitialized;

  bool m_threadRunning;

  bool m_channelsConnected;

  int m_curChannelNumber;

  // TODO i use future/promise for thread control, but i'm starting to think
  // thats not appropriate
  std::promise<void>* m_exitThreadSignal;
  std::future<void> m_futureObj;

  void thread();

  int addExchange(const std::string& exchange);
  int addExchange(const std::string& exchange, const std::string& type);

  std::mutex m_producerMutex;

  std::thread m_producerThread;

  std::queue<std::shared_ptr<helper::RawMessage> > m_sendQueue;

 public:
  Producer()
      : m_exchange(""),
        m_isInitialized(false),
        m_threadRunning(false),
        m_channelsConnected(false),
        m_curChannelNumber(1),
        m_exitThreadSignal(nullptr){};

  // Constructors with exchanges defined, may be thrown out TODO
  Producer(std::string&& exchange)
      : m_exchange(std::move(exchange)),
        m_isInitialized(false),
        m_threadRunning(false),
        m_channelsConnected(false),
        m_curChannelNumber(1),
        m_exitThreadSignal(nullptr){};

  Producer(const std::string& exchange)
      : m_exchange(exchange),
        m_isInitialized(false),
        m_threadRunning(false),
        m_channelsConnected(false),
        m_curChannelNumber(1),
        m_exitThreadSignal(nullptr){};

  HARE_ERROR_E Send(const std::string& routing_value, Message& message);

  HARE_ERROR_E Send(const std::string& exchange,
                    const std::string& routing_value, Message& message);

  void SetExchange(const std::string& exchange);

  HARE_ERROR_E DeclareExchange(const std::string& exchange,
                               const std::string& type = "direct");

  HARE_ERROR_E Start();
  HARE_ERROR_E Stop();

  HARE_ERROR_E Restart();

  // TODO can i delete this?
  void operator()() { thread(); };

  /**
   * Intialize function for no default exchange
   */
  HARE_ERROR_E Initialize(const std::string& server, int port);

  /**
   * Copy Constructor
   */
  Producer(const Producer&) = delete;

  /**
   * Destructor
   */
  ~Producer();

  bool IsInitialized() { return m_isInitialized; }
};

}  // Namespace HareCpp

#endif /*PRODUCER_H*/
