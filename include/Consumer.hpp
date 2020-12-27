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

/**
 * Consumer class uses ChannelHandler to keep track of callbacks and channel
 * information, while using ConnectionBase to connect to rabbitmq broker. It
 * consumes messages created by Producer class (or other rabbitmq amqp
 * implementation).
 *
 * Upon constructing the object, you must run Initialize to initialize the
 * default rabbitmq broker credentials (SSL To be done later TODO). From there
 * you subscribe to different exchange/routing key combinations and assign a
 * callback function to these subscriptions (to be called once consumed).
 *
 * After initialization you can Start() the class, which spins off one or two
 * threads.  Two threads are used when not every exchange can be connected to
 * (producer hasn't declared them), in which a thread will constently be trying
 * to establish a connection, until it is established and that secondary thread
 * will die. The first, and main subscribe thread, will continue to run until
 * Restart(), Stop(), or general deconstruction happens.
 *
 */
class Consumer {
 private:
  /**
   * ChannelHandler class acts as a helper to keep track of exchanges/routing
   * keys and their associated callback function. It acts as a 'smarter' way to
   * keep track of these objects and give Consumer an easier way to access and
   * set them.
   *
   * There is no need for another channelHandler, so only one is created by
   * default.
   */
  ChannelHandler m_channelHandler;

  /**
   * The status of initialization of the Consumer, if certain variables/structs
   * are not set no connection to the rabbitmq broker can be established. This
   * boolean is periodically checked in Consumer calls to make sure that we can
   * continue processing.
   */
  bool m_isInitialized;

  /**
   * ConnectionBased used to establish and keep track of connection to the
   * rabbitmq broker.  It is the gatekeeper for all amqp calls. Currently a
   * shared_ptr due to the possibility of using different types of Connection
   * implementations.  Though currently Base is the only one available
   */
  std::shared_ptr<connection::ConnectionBase> m_connection;

  /**
   * Mutex used throughout the class
   */
  mutable std::mutex m_consumerMutex;

  /**
   * bools to determine if the 2 main threads are already running
   */
  bool m_threadRunning;
  bool m_unboundChannelThreadRunning;

  /**
   * Trying out c++11 techniques, this may be redundant as i think its main
   * appeal is for accessing threads that are detached from the class. Because
   * our threads have access to member variables, the booleans above may be all
   * we need.  TODO
   */
  std::promise<void>* m_unboundChannelThreadSig;
  std::future<void> m_futureObjUnboundChannel;

  /**
   * Start up all channels and start consuming on them.  This method uses
   * setupAndConsume() to do creation and binding of the queues.  It also holds
   * the mutex to keep setupAndConsume thread safe (it itself doesn't use
   * mutexes).
   */
  HARE_ERROR_E startConsumption();

  /**
   * Two main threads that could run through the lifetime of Consumer
   */
  void thread();
  void unboundChannelThread();
  std::thread m_consumerThread;
  std::thread m_unboundChannelThread;

  /**
   *  Binds and consumes a queue/exchange
   *  If a channel exception is received, the channel is added to
   *  m_unboundChannels queue to be periodically tried again
   *
   *  NOTE: does not use a mutex, as it is assumed the caller method will hold
   * the lock.  This is not thread safe!
   *
   * @param [in] channel : the channel to establish to the broker
   *
   * @returns HARE_ERROR_E : the success of the binding or the server failure
   * status
   */
  HARE_ERROR_E setupAndConsume(int channel);

  /**
   * Opens a channel using the ConnectionBase class
   *
   * @param [in] channel : the channel to establish the connection
   * @returns HARE_ERROR_E : server failure or general connection issues
   *
   */
  HARE_ERROR_E openChannel(const int channel);

  /**
   * declares a queue using the ConnectionBase class
   *
   * @param [in] channel : the channel to declare the queue on
   * @param [out] queueName : the name of the queue to be used later to
   * bind/read on
   *
   * @returns HARE_ERROR_E : server failure or general connection issues
   *
   */
  HARE_ERROR_E declareQueue(const int channel, amqp_bytes_t& queueName);

  /**
   * binds to a queue using the ConnectionBase class
   *
   * @param [in] channel : the channel to bind the queue on
   * @param [in] queueName : the name of the queue to be used to bind
   *
   * @returns HARE_ERROR_E : server failure or general connection issues
   *
   */
  HARE_ERROR_E bindQueue(const int channel, const amqp_bytes_t& queueName);

  /**
   * starts consumption on a channel using the queueName declared/bound
   * a prerequisite is that the former has already been called.
   *
   * @param [in] channel : the channel to be consumed on
   * @param [in] queueName : the name of the queue to be used to read from the
   * channel
   *
   * @returns HARE_ERROR_E : server failure or general connection issues
   *
   */
  HARE_ERROR_E consume(const int channel, const amqp_bytes_t& queueName);

  /**
   * Connect to the broker and starts consumption on all queues it can bind to.
   * Any queues unable to create or bind (possibly due to exchange not being
   * created yet), will be put on unbound channel thread (see
   * startConsumption()).
   *
   * If a server failure is encountered (possibly due to broker not being set up
   * yet), a sleep will happen.  This is to stop the broker from being flooded
   * with requests during its startup, which will speed up it being up and
   * running and allowing connection. Believe it or not, this sleep does improve
   * time to reestablish a connection.
   *
   * @returns HARE_ERROR_E : results of the connect and consumption
   *
   */
  HARE_ERROR_E connectAndStartConsumption();

  /**
   * pullNextMessage consumes/pulls the next message on the list from all queues
   * we are bound to and subscribing to. It will then use the channelHandler to
   * process the message, calling the callback function assigned to that
   * exchange/route.
   *
   * @returns void
   */
  void pullNextMessage();

  /**
   * Queue of unbound channels that need to be retried in the
   * unboundChannelThread.  They will continue to go in and out of the queue
   * until everything has been successfully connected to the broker.  This
   * exists, along with the thread, as a means to make sure that we don't lose
   * connection if an exchange hasn't been declared.
   */
  std::queue<int> m_unboundChannels;

  /**
   * Similar to Stop() call, except created to kill off the
   * unboundChannelThread.  Though this may be redundant. TODO
   */
  void stopUnboundChannelThread();

  /**
   * Similar to Start() call, except created to start up the
   * unboundChannelThread. This thread runs and retries all channels/queues that
   * are not being subscribed on
   */
  void startUnboundChannelThread();

  /**
   * setRunning used to set m_threadRunning using mutex
   *
   * @param [in] running (bool)
   */
  void setRunning(bool running);

 public:
  /**
   * Default constructor
   */
  Consumer()
      : m_isInitialized(false),
        m_threadRunning(false),
        m_unboundChannelThreadRunning(false),
        m_unboundChannelThreadSig(nullptr){};

  /**
   * Start() and Stop() the main consumer thread
   *
   * @returns HARE_ERROR_E return code, check for
   * HareCpp::HARE_ERROR_E::ALL_GOOD
   */
  HARE_ERROR_E Start();
  HARE_ERROR_E Stop();

  /**
   * Subscribe to an exchange and binding/routing key.  Use queue properties to
   * define the queue being generated and a callback function to be called upon
   * consumption.
   *
   * @param [in] exchange : the name of the exchange we are subscribing to.
   * @param [in] binding_key : the binding key to a particular exchange route.
   * @param [in] f : callback function (void CALLBACK(const HareCpp::Message&
   * message)).
   * @param [in] queueProps : HareCpp::helper::queueProperties that can be
   * created by the user to define the properties the queue should have upon
   * creation.
   */
  HARE_ERROR_E Subscribe(
      const std::string& exchange, const std::string& binding_key,
      TD_Callback f,
      helper::queueProperties queueProps = helper::queueProperties());

  /**
   * Intialize function
   *
   * @param optional [in] server : the server/host of the rabbitmq broker.
   * @param optional [in] port : the port used by the rabbitmq broker.
   * @param optional [in] username : the username to be used by rabbitmq
   * connection.
   * @param optional [in] password : the password to be used by rabbitmq
   * connection.
   *
   * @returns HARE_ERROR_E
   */
  HARE_ERROR_E Initialize(const std::string& server = "localhost",
                          int port = 5672,
                          const std::string& username = "guest",
                          const std::string& password = "guest");

  /**
   * Restart function
   *
   * Turns off the thread, then restarts it.  Used to reestablish a connection
   * to the broker if failed.  Helpful command when a new subscription has been
   * created and a new connection needs to be established.
   */
  HARE_ERROR_E Restart();

  /**
   * Copy Constructor
   *
   * Delete
   */
  Consumer(const Consumer&) = delete;

  /**
   * Destructor
   *
   * Close connection and stop main threads
   */
  ~Consumer() {
    if (m_threadRunning) Stop();
    if (m_isInitialized) m_connection->CloseConnection();
    m_isInitialized = false;
  };

  bool IsInitialized() const {
    const std::lock_guard<std::mutex> lock(m_consumerMutex);
    return m_isInitialized;
  }

  bool IsRunning() const {
    const std::lock_guard<std::mutex> lock(m_consumerMutex);
    return m_threadRunning;
  }
};

}  // Namespace HareCpp

#endif /*CONSUMER_H*/