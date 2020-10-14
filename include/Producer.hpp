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

/**
 * Producer class is used to produce amqp messages (currently as std::strings)
 * and deliver them to a rabbitmq broker. It uses ConnectionBase, similar to
 * Consumer, to establish the connections and gives user a simple API in order
 * to access and send messages.
 *
 * Upon constructing the object, you must Initialize() to set the default
 * rabbitmq broker credentials (SSL to be done later TODO). From there you can
 * set a default exchange to write to, or simply send messages to specific
 * exchange/routing keys manually.
 *
 * Declare all exchanges (except default ones i.e amq.direct) prior to using
 * them, as the consumer will not be able to establish connection, though it
 * seems no error will be returned to the producer producing to an undeclared
 * exchange.
 *
 * NOTE: Currently exchange characteristics/properties have not been
 * implemented, so you can define the type of exchange but that is about all
 * (TODO).
 *
 * NOTE: Keeping track of last used exchange is nice, but not super
 * necessarily and I believe makes this class a bit harder to follow because of
 * it.  I might remove (TODO)
 *
 */
class Producer {
 private:
  /**
   * ExchangeProperties is a private struct to keep track of exchange and their
   * characteristics. This allows an easier time to find their information when
   * establishing connection to the broker, as they are all declared/opened up
   * once a connection is established (Start() function is called)
   *
   * @param optional [in] channel : channel used for this exchange
   *
   * @param optional [in] isDeclare : when connection is established, this flag
   * will be used to determine if we need to declare this exchange prior to
   * using/connecting to it.
   *
   * @param optional [in] type : type of exchange we are using.  Defaults to
   * "direct"
   *
   */
  struct ExchangeProperties {
    ExchangeProperties(int channel = -1, bool isDeclare = false,
                       const std::string& type = "direct")
        : m_channel(channel),
          m_connected(false),
          m_isDeclare(isDeclare),
          m_type(type){};

    int m_channel;
    bool m_connected;

    bool m_isDeclare;  // Do we need to declare this exchange?
    std::string m_type;
  };

  /**
   *  Clears the queue of messages to be sent out, freeing the memory
   */
  void clearActiveSendQueue();

  /**
   * Closes the connection after a failure, to be retried by active producer
   * thread
   */
  void closeConnection();

  /**
   * Hashmap to easily find exchange information when sending
   * messages/establishing connections
   */
  std::unordered_map<std::string, ExchangeProperties> m_exchangeList;

  /**
   * ConnectionBased used to establish and keep track of connection to the
   * rabbitmq broker.  It is the gatekeeper for all amqp calls. Currently a
   * shared_ptr due to the possibility of using different types of Connection
   * implementations.  Though currently Base is the only one available
   */
  std::shared_ptr<connection::ConnectionBase> m_connection;

  /**
   * Last used exchange (TODO may be removed).  This is so that, in case you
   * only use one exchange, you can set it up and then send on simply the
   * routing key only. Though, lets be real, this is just weird and makes
   * following the class a little harder. So probably will be removed.
   */
  std::string m_exchange;

  /**
   * Stateful variables
   */
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

  /**
   *  Publish the next message in the m_sendQueue
   */
  void publishNextInQueue();

  /**
   * Create and connect all exchanges to send on to a unique channel ID.
   * If the exchange needs to be declared, this is where it will happen.
   */
  void connectChannels();

  bool channelsConnected() const;

  mutable std::mutex m_producerMutex;

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
  explicit Producer(std::string&& exchange)
      : m_exchange(std::move(exchange)),
        m_isInitialized(false),
        m_threadRunning(false),
        m_channelsConnected(false),
        m_curChannelNumber(1),
        m_exitThreadSignal(nullptr){};

  explicit Producer(const std::string& exchange)
      : m_exchange(exchange),
        m_isInitialized(false),
        m_threadRunning(false),
        m_channelsConnected(false),
        m_curChannelNumber(1),
        m_exitThreadSignal(nullptr){};

  /**
   * Sends a message given the routing key.  This uses the last used exchange as
   * the exchange for the message to be sent out. The message gets converted
   * over to HareCpp::helper::RawMessage structure to be put on a queue to be
   * sent out in the main Producer thread (assuming its been started).
   *
   * @param [in] routing_value : the routing key used to distribute out the
   * message on the last used exchange
   *
   * @param [in] message : the HareCpp::Message with the contents to be sent out
   *
   * @returns HARE_ERROR_E error code
   */
  HARE_ERROR_E Send(const std::string& routing_value, Message& message);

  /**
   * Sends a message given both the exchange and routing key used.  The exchange
   * set by this call will be the default for the next call of the former
   * Send(routing_value,message) declaration.  Though, this may change later.
   * The message given gets converted to a HareCpp::helper::RawMessage structure
   * to be put on a queue to be sent out in the main Producer thread (assuming
   * its been started).
   *
   * @param [in] exchange : the rabbitmq exchange the message is sent on
   *
   * @param [in] routing_value : the routing key used to route the message on
   * the exchange
   *
   * @param [in] message : the HareCpp::Message with the contents to be sent out
   *
   * @returns HARE_ERROR_E error code
   */
  HARE_ERROR_E Send(const std::string& exchange,
                    const std::string& routing_value, Message& message);

  /**
   *  Sets the default used exchange for sending messages, this is also set when
   * a Send call is made with the exchange as a parameter. Again, this may be
   * removed in future iterations as it doesn't seem very necessary and instead
   * adds to complexity and confusion of maintanence.
   *
   * @param [in] exchange : the rabbitmq exchange to be made default
   *
   * @returns void
   */
  void SetExchange(const std::string& exchange);

  HARE_ERROR_E DeclareExchange(const std::string& exchange,
                               const std::string& type = "direct");

  HARE_ERROR_E Start();
  HARE_ERROR_E Stop();

  HARE_ERROR_E Restart();

  /**
   * Intialize function for no default exchange
   */
  HARE_ERROR_E Initialize(const std::string& server = "localhost",
                          int port = 5672,
                          const std::string& username = "guest",
                          const std::string& password = "guest");

  /**
   * Copy Constructor
   */
  Producer(const Producer&) = delete;

  /**
   * Used to see how many messages are still left to send, helpful if user wants to turn off
   * producer cleanly without loss of messages.
   * 
   * @returns size of send queue.
   */
  int QueueSize() const;

  /**
   * Destructor
   */
  ~Producer();

  bool IsInitialized() const;
  bool IsRunning() const;
};

}  // Namespace HareCpp

#endif /*PRODUCER_H*/
