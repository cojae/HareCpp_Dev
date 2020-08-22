
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
#ifndef _CONNECTION_BASE_H_
#define _CONNECTION_BASE_H_

#include "HelperStructs.hpp"
#include "pch.hpp"

namespace HareCpp {
namespace connection {
/**
 *  ConnectionBase class is the main class used by Producer/Consumer classes (or
 * other future classes/apis) in order to streamline the use of rabbitmq-c api
 * calls to the rabbitmq broker (or other type of broker). This class allows
 * abstraction of the connection away from the client classes
 * (producer/consumer), and will hopefully ease their usage and allow easier
 * expansion later on.
 *
 */
class ConnectionBase {
 protected:
  amqp_socket_t* m_socket;
  /**
   * Rabbitmq-c connection state that is used for every amqp connection call.
   * There should only be one of these per publisher/consumer processing, and
   * needs to be properly mutex locked. This is why the ConnectionBase class is
   * important, to handle this.
   */
  amqp_connection_state_t m_conn;

  /**
   * Mutex lock on the amqp_connection_state_t, amongst other important things
   * within ConnectionBase
   */
  mutable std::mutex m_connMutex;

  /**
   * Stored credentials for login, this includes username, password, host, and
   * port of the rabbitmq broker They default to : Username: "guest" Password:
   * "guest" Host:     "localhost" Port:      5672
   */
  helper::loginCredentials m_basicCredentials;

  /**
   * The stored SSL credentials.  This will only be used when ssl connection is
   * necessary and will change the default Connect() function call to connect
   * via SSL
   *
   * TODO: Unimplemented currently
   */
  helper::sslCredentials m_sslCredentials;

  /**
   * boolean for whether or not the amqp_connection_state_t has been established
   * This will be returned for IsConnected() calls and as a double check before
   * using an existing connection that may be inactive/broken
   */
  bool m_isConnected;

  /**
   * TODO: Is this still used? Can this be removed
   */
  bool m_isTryingConnection;

  /**
   * Wether or not the SSL connection credentials have been set, and if we need
   * to be using SSL when establishing any new connections
   */
  bool m_isSSL;

  // We need to reestablish connection, an exception was caught
  /** TODO: is this still used? */
  bool m_connectionFailure;

  /**
   * Timeout used during consumption of a channel/ any amqp call that may
   * include a timeout or lock the resource.
   */
  int m_timeout;

  /**
   * login using the basic login credentials given in the class' constructor
   *
   * @returns HARE_ERROR_E
   */
  HARE_ERROR_E login();

  /**
   * basic non-SSL connection to the rabbitmq broker.
   * This is the default connection, and requires SSL credentials to be given in
   * order to use SSL
   *
   * @returns HARE_ERROR_E
   */
  HARE_ERROR_E connectBasic();
  /**
   *  NOT IMPLEMENTED TODO
   * @returns HARE_ERROR_E
   */
  HARE_ERROR_E connectSSL();

  /**
   * Helper function to decode the rpc reply into a HARE_ERROR_E to be used by
   * user of this class
   * @returns HARE_ERROR_E
   */
  HARE_ERROR_E decodeRpcReply(const amqp_rpc_reply_t& reply);

  HARE_ERROR_E decodeLibraryException(const amqp_rpc_reply_t& reply);

 public:
  /**
   * Default connection base constructor.  It takes in basic credentials to
   * login to the rabbitmq broker. If nothing is given, the defaults are used:
   * guest:guest localhost:5672
   */
  ConnectionBase(const std::string& hostname = "localhost", int port = 5672,
                 const std::string& username = "guest",
                 const std::string& password = "guest");

  /**
   * Whether or not a connection to the rabbitmq broker is established
   *
   * @returns m_isConnected boolean (true=connected,false=not connected)
   */
  bool IsConnected();

  /**
   * Create the timeout time used when calling amqp functions that allow a
   * timeout to occur Without a timeout, these calls will sometimes block
   * indefinetly
   *
   * This is currently in seconds, but will need to be added for more precision
   * (TODO)
   */
  void SetTimeout(int timeout);

  /**
   * Connect, which calls either basic or ssl connection functions
   *
   * @returns HARE_ERROR_E
   */
  HARE_ERROR_E Connect();

  /**
   * Close Connection to the rabbitmq broker
   *
   * @returns HARE_ERROR_E
   */
  HARE_ERROR_E CloseConnection();

  /**
   * Open a channel to the broker using the channel number provided
   *
   * @param [in] channel : channel to be opened
   * @returns HARE_ERROR_E with success or not
   */
  HARE_ERROR_E OpenChannel(int channel);

  /**
   * Close the channel to the broker with the channel number provided
   *
   * @param [in] channel : channel to be closed
   * @returns HARE_ERROR_E with success or not
   */
  HARE_ERROR_E CloseChannel(int channel);

  /**
   * Declare an exchange on given channel with the exchange name and type given
   *
   * @param [in] channel : active channel used to declare the exchange
   * @param [in] exchange : name of the exchange being created
   * @param [in] type : type of exchange as string "direct","topic",etc
   * @returns HARE_ERROR_E with success or not
   */
  HARE_ERROR_E DeclareExchange(int channel, const std::string& exchange,
                               const std::string& type);

  /**
   * Publish a RawMessage struct using its exchange/routing key
   *
   * @param [in] message : RawMessage containing the message and associated
   * publish information
   * @returns HARE_ERROR_E with success or not
   */
  HARE_ERROR_E PublishMessage(helper::RawMessage& message);

  /**
   * Consume a message, filling the amqp_envelope_t with the contents of the
   * message received This does not work in the case of not receiving a full
   * envelope at once, receiving partial frames. This will eventually need to be
   * updated accordingly for this type of scenario.  But for now, we assume it
   * all comes through correctly.
   *
   * @param [out] envelope : contains the message consumed from the amqp broker
   * @returns HARE_ERROR_E with success or not
   */
  HARE_ERROR_E ConsumeMessage(amqp_envelope_t& envelope);

  /**
   * Turn on amqp consumption on the channel/queue.
   * This calls underlying amqp_consume function which starts up consumption. It
   * relies, however, on queueName being already declared and associated with an
   * exchange/routing key already.  Once consumption has started, ConsumeMessage
   * will return messages for that queue, and any other queue that consumption
   * has been called on via this function
   *
   * @param [in] channel : channel associated with the queue
   * @param [in] queueName : name of the queue to be used for consumption
   * @returns HARE_ERROR_E with success or not
   */
  HARE_ERROR_E StartConsumption(int channel, amqp_bytes_t queueName);

  /**
   * Declare a queue on a channel, given the queueProperies that could be set by
   * the user/library caller
   *
   * @param [in] channel : channel used to declare queue on
   * @param [in] queueProps : struct containing properties of the queue
   * (durability, etc)
   * @param [out] retQueue : the queue name created/declared.  To be used in
   * StartConsumption() call
   * @returns HARE_ERROR_E with success or not
   */
  HARE_ERROR_E DeclareQueue(int channel,
                            const helper::queueProperties& queueProps,
                            amqp_bytes_t& retQueue);

  /**
   * Binds a declared queue to a particular exchange/binding key.  The broker
   * will then use that queue to write messages to be consumed This happens when
   * the StartConsumption call is made
   *
   * @param [in] channel : the channel to use for the queue/consumption
   * @param [in] queueName : the name of the queue that had been declared in
   * DeclareQueue() call
   * @param [in] exchange : the name of the exchange we are binding to
   * @param [in] bindingKey : the binding/routing key on the exchange we care
   * about
   * @returns HARE_ERROR_E with success or not
   */
  HARE_ERROR_E BindQueue(int channel, const amqp_bytes_t& queueName,
                         const std::string& exchange,
                         const std::string& bindingKey);

  /**
   * May not be necessary anymore, but returns a pointer to the current
   * connection state
   *
   * @returns amqp_connection_state_t m_conn
   */
  amqp_connection_state_t& Connection();

  /**
   * Destructor should close connections before being destroyed
   */
  ~ConnectionBase() { CloseConnection(); };
};  // Class ConnectionBase
}  // Namespace connection
}  // Namespace HareCpp

#endif  // _CONNECTION_BASE_H_