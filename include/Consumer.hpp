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

#ifndef _CONSUMER_H_
#define _CONSUMER_H_

#include "ChannelHandler.hpp"
#include "ConnectionBase.hpp"
#include "Message.hpp"
#include "pch.hpp"

#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace HareCpp {

class Consumer {
 private:
  ChannelHandler m_channelHandler;
  bool m_isInitialized;

  std::shared_ptr<connection::ConnectionBase> m_connection;

  std::mutex m_consumerMutex;
  std::mutex m_connectionMutex;

  bool m_threadRunning;
  bool m_unboundChannelThreadRunning;

  std::promise<void>* m_exitThreadSignal;
  std::future<void> m_futureObj;

  std::promise<void>* m_unboundChannelThreadSig;
  std::future<void> m_futureObjUnboundChannel;

  /**
   * Two main threads that could run through the lifetime of Consumer
   */
  void thread();
  void unboundChannelThread();
  std::thread m_consumerThread;
  std::thread m_unboundChannelThread;

  /**
   *  Binds to a queue/exchange
   *  If a channel exception is received, the channel is added to
   *  m_unboundChannels queue to be periodically tried again
   */
  int smartBind(int channel);

  std::queue<int> m_unboundChannels;
  void stopUnboundChannelThread();

  void privateRestart();

 public:
  Consumer() : m_isInitialized(false), 
               m_threadRunning(false), 
               m_unboundChannelThreadRunning(false),
               m_exitThreadSignal(nullptr),
               m_unboundChannelThreadSig(nullptr)  { };

  HARE_ERROR_E start();
  HARE_ERROR_E stop();

  // TODO can i delete this?
  void operator()() { thread(); }

  HARE_ERROR_E subscribe(
      const std::string& exchange, const std::string& binding_key,
      TD_Callback f,
      helper::queueProperties queueProps = helper::queueProperties());

  /**
   * Intialize function
   */
  HARE_ERROR_E Initialize(const std::string& server, int port);

  /**
   * Restart function
   */
  HARE_ERROR_E Restart();

  /**
   * Copy Constructor
   */
  Consumer(const Consumer&) = delete;

  /**
   * Destructor
   */
  ~Consumer() {
    stop();
    m_connection->CloseConnection();
  };

  bool isInitialized() { 
    const std::lock_guard<std::mutex> lock(m_consumerMutex); 
    return m_isInitialized; 
  }

  bool isRunning() { 
    const std::lock_guard<std::mutex> lock(m_consumerMutex); 
    return m_threadRunning; 
  }
};

}  // Namespace HareCpp

#endif /*CONSUMER_H*/