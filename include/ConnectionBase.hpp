
#ifndef _CONNECTION_BASE_H_
#define _CONNECTION_BASE_H_

#include "HelperStructs.hpp"
#include "pch.hpp"

namespace HareCpp {
namespace connection {
/**
 *  ConnectionBase class is the main class used by Producer/Consumer classes (or other future classes/apis)
 *  in order to streamline the use of rabbitmq-c api calls to the rabbitmq broker (or other type of broker).
 *  This class allows abstraction of the connection away from the client classes (producer/consumer), and will hopefully ease their usage and allow easier expansion later on.
 * 
 */
class ConnectionBase {
 protected:
  amqp_socket_t* m_socket;
  /**
   * Rabbitmq-c connection state that is used for every amqp connection call.
   * There should only be one of these per publisher/consumer processing, and needs to be properly mutex locked.
   * This is why the ConnectionBase class is important, to handle this.
   */
  amqp_connection_state_t m_conn;

  /**
   * Mutex lock on the amqp_connection_state_t, amongst other important things within ConnectionBase
   */
  mutable std::mutex m_connMutex;

  /**
   * Stored credentials for login, this includes username, password, host, and port of the rabbitmq broker
   * They default to :
   *    Username: "guest" 
   *    Password: "guest" 
   *    Host:     "localhost" 
   *    Port:      5672
   */
  helper::loginCredentials m_basicCredentials;

  /**
   * The stored SSL credentials.  This will only be used when ssl connection is necessary and will change the default Connect() function call to connect via SSL
   * 
   * TODO: Unimplemented currently
   */
  helper::sslCredentials m_sslCredentials;

  /**
   * boolean for whether or not the amqp_connection_state_t has been established
   * This will be returned for IsConnected() calls and as a double check before using an existing connection that may be inactive/broken
   */
  bool m_isConnected;

  /**
   * TODO: Is this still used? Can this be removed
   */
  bool m_isTryingConnection;

  /**
   * Wether or not the SSL connection credentials have been set, and if we need to be using SSL when establishing any new connections
   */
  bool m_isSSL;

  // We need to reestablish connection, an exception was caught
  /** TODO: is this still used? */
  bool m_connectionFailure;

  /**
   * Timeout used during consumption of a channel/ any amqp call that may include a timeout or lock the resource.  
   */
  int m_timeout;

  HARE_ERROR_E logIn();

  HARE_ERROR_E connectBasic();
  HARE_ERROR_E connectSSL();

  HARE_ERROR_E decodeRpcReply(amqp_rpc_reply_t reply);

 public:
  ConnectionBase(const std::string& hostname = "localhost", int port = 5672,
                 const std::string& username = "guest",
                 const std::string& password = "guest");

  bool IsConnected();

  void SetTimeout(int timeout);

  HARE_ERROR_E Connect();

  HARE_ERROR_E CloseConnection();

  HARE_ERROR_E OpenChannel(int channel);

  HARE_ERROR_E CloseChannel(int channel);

  HARE_ERROR_E DeclareChannel(int channel, const std::string& exchange,
                              const std::string& type);

  HARE_ERROR_E PublishMessage(helper::RawMessage& message);

  HARE_ERROR_E ConsumeMessage(amqp_envelope_t& envelope);

  HARE_ERROR_E StartConsumption(int channel, amqp_bytes_t queueName);

  HARE_ERROR_E DeclareQueue(int channel,
                            const helper::queueProperties& queueProps,
                            amqp_bytes_t& retQueue);

  HARE_ERROR_E BindQueue(int channel, const amqp_bytes_t& queueName,
                         const std::string& exchange,
                         const std::string& bindingKey);

  amqp_connection_state_t& Connection();

  ~ConnectionBase() { CloseConnection(); };
};  // Class ConnectionBase
}  // Namespace connection
}  // Namespace HareCpp

#endif  // _CONNECTION_BASE_H_