#include "ConnectionBase.hpp"

namespace HareCpp {
namespace connection {

bool ConnectionBase::IsConnected() {
  const std::lock_guard<std::mutex> lock(m_connMutex);
  return m_isConnected;
}

HARE_ERROR_E ConnectionBase::login() {
  const std::lock_guard<std::mutex> lock(m_connMutex);
  LOG(LOG_DETAILED, "Attempting to log in to rabbitMQ  broker");
  auto retCode = HARE_ERROR_E::ALL_GOOD;
  auto amqpReply = amqp_login(m_conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN,
                              m_basicCredentials.m_username.c_str(),
                              m_basicCredentials.m_password.c_str());
  if (amqpReply.reply_type != 1) {
    LOG(LOG_FATAL, "Unable to log into rabbitMQ broker");
    retCode = HARE_ERROR_E::SERVER_AUTHENTICATION_FAILURE;
  }
  return retCode;
}

amqp_connection_state_t& ConnectionBase::Connection() {
  const std::lock_guard<std::mutex> lock(m_connMutex);
  return m_conn;
}

ConnectionBase::ConnectionBase(const std::string& hostname, int port,
                               const std::string& username,
                               const std::string& password)
    : m_socket(nullptr),
      m_basicCredentials(hostname, port, username, password),
      m_isConnected(false),
      m_isTryingConnection(false),
      m_isSSL(false),
      m_connectionFailure(false),
      m_timeout(DEFAULT_CONNECTION_TIMEOUT) {}

HARE_ERROR_E ConnectionBase::CloseConnection() {
  auto retCode = HARE_ERROR_E::ALL_GOOD;
  const std::lock_guard<std::mutex> lock(m_connMutex);
  if (m_isConnected) {
    LOG(LOG_WARN, "Closing Connection");

    auto amqpReply = amqp_connection_close(m_conn, AMQP_REPLY_SUCCESS);
    switch (amqpReply.reply_type) {
      case AMQP_RESPONSE_NORMAL:
        break;
      case AMQP_RESPONSE_NONE:
        LOG(LOG_ERROR, "Missing RPC Reply from amqp_connection_close");
        retCode = HARE_ERROR_E::NO_RPC_REPLY;
        break;
      case AMQP_RESPONSE_SERVER_EXCEPTION:
        LOG(LOG_FATAL, "Server exception in amqp_connection_close");
        retCode = HARE_ERROR_E::SERVER_EXCEPTION_RESPONSE;
        m_connectionFailure = true;
        break;
      default:
        break;
    }
    amqp_destroy_connection(m_conn);
  }

  m_isConnected = false;
  return retCode;
}

void ConnectionBase::SetTimeout(int timeout) {
  const std::lock_guard<std::mutex> lock(m_connMutex);
  m_timeout = timeout;
}

HARE_ERROR_E ConnectionBase::connectBasic() {
  auto retCode = HARE_ERROR_E::ALL_GOOD;

  m_socket = nullptr;

  const std::lock_guard<std::mutex> lock(m_connMutex);
  m_conn = amqp_new_connection();
  m_socket = amqp_tcp_socket_new(m_conn);
  if (m_socket == nullptr) {
    LOG(LOG_FATAL, "Failed to create socket to amqp Connection");
    retCode = HARE_ERROR_E::INITIALIZE_FAILURE;
  }

  if (noError(retCode)) {
    LOG(LOG_DETAILED, "Attempting to connect to TCP Socket");
    struct timeval timeout = {2, 0};
    auto status = amqp_socket_open_noblock(
        m_socket, m_basicCredentials.m_hostname.c_str(),
        m_basicCredentials.m_port, &timeout);
    if (status) {
      LOG(LOG_FATAL, "Unable to open TCP Socket to hostname");
      retCode = HARE_ERROR_E::SERVER_CONNECTION_FAILURE;
    } else {
      LOG(LOG_INFO, "Opened TCP Socket to broker");
    }
  }

  return retCode;
}

HARE_ERROR_E ConnectionBase::Connect() {
  auto retCode = HARE_ERROR_E::ALL_GOOD;

  // Set up connection information
  if (m_isSSL) {
    // TODO ssl connection
  } else {
    retCode = connectBasic();
  }

  if (noError(retCode)) {
    retCode = login();
  }

  if (noError(retCode)) {
    m_isConnected = true;
  }

  return retCode;
}

HARE_ERROR_E ConnectionBase::DeclareExchange(int channel,
                                             const std::string& exchange,
                                             const std::string& type) {
  const std::lock_guard<std::mutex> lock(m_connMutex);
  if (false == m_isConnected) {
    return HARE_ERROR_E::SERVER_CONNECTION_FAILURE;
  }
  amqp_exchange_declare(m_conn, channel, amqp_cstring_bytes(exchange.c_str()),
                        amqp_cstring_bytes(type.c_str()), 0, 0, 0, 0,
                        amqp_empty_table);  // TODO MORE POWER TO USER
  return (decodeRpcReply(amqp_get_rpc_reply(m_conn)));
}

HARE_ERROR_E ConnectionBase::OpenChannel(int channel) {
  auto retCode = HARE_ERROR_E::ALL_GOOD;
  const std::lock_guard<std::mutex> lock(m_connMutex);

  if (false == m_isConnected) {
    return HARE_ERROR_E::SERVER_CONNECTION_FAILURE;
  }

  amqp_channel_open(m_conn, channel);

  retCode = decodeRpcReply(amqp_get_rpc_reply(m_conn));

  if (retCode == HARE_ERROR_E::CHANNEL_EXCEPTION) {
    amqp_channel_close_ok_t close_ok;
    auto res = amqp_send_method(m_conn, channel, AMQP_CHANNEL_CLOSE_OK_METHOD,
                                &close_ok);
    if (res != AMQP_STATUS_OK) {
      LOG(LOG_FATAL, "Unable to close channel");
    }
  }

  return retCode;
}

HARE_ERROR_E ConnectionBase::CloseChannel(int channel) {
  auto retCode = HARE_ERROR_E::ALL_GOOD;
  const std::lock_guard<std::mutex> lock(m_connMutex);

  if (false == m_isConnected) {
    return HARE_ERROR_E::SERVER_CONNECTION_FAILURE;
  }

  amqp_channel_close(m_conn, channel, AMQP_REPLY_SUCCESS);

  // TODO this is always ALL_GOOD
  return retCode;
}

HARE_ERROR_E ConnectionBase::PublishMessage(helper::RawMessage& message) {
  auto retCode = HARE_ERROR_E::ALL_GOOD;
  const std::lock_guard<std::mutex> lock(m_connMutex);

  if (false == m_isConnected) {
    return HARE_ERROR_E::SERVER_CONNECTION_FAILURE;
  }

  // If timestamp isn't set, set it here
  if (AMQP_BASIC_TIMESTAMP_FLAG !=
      (message.properties._flags & AMQP_BASIC_TIMESTAMP_FLAG)) {
    message.properties._flags |= AMQP_BASIC_TIMESTAMP_FLAG;
    auto curTimeInMicroSecs =
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();
    message.properties.timestamp = curTimeInMicroSecs;
  }

  auto errorVal = amqp_basic_publish(m_conn, message.channel, message.exchange,
                                     message.routing_value, 0, 0,
                                     &message.properties, message.message);

  if (errorVal < 0) {
    LOG(LOG_ERROR, amqp_error_string2(errorVal));
    // TODO split the decodeReply function into two, and use the status check
    // for this
    if (errorVal == AMQP_STATUS_SOCKET_ERROR) {
      retCode = HARE_ERROR_E::SERVER_CONNECTION_FAILURE;
    } else {
      retCode = HARE_ERROR_E::PUBLISH_ERROR;
    }
  }

  return retCode;
}
HARE_ERROR_E ConnectionBase::ConsumeMessage(amqp_envelope_t& envelope) {
  const std::lock_guard<std::mutex> lock(m_connMutex);

  // TODO allow millisecs/seconds to be used here
  // Currently just does seconds
  struct timeval timeout = {m_timeout, 0};

  return decodeRpcReply(amqp_consume_message(m_conn, &envelope, &timeout, 0));
}

HARE_ERROR_E ConnectionBase::StartConsumption(int channel,
                                              amqp_bytes_t queueName) {
  const std::lock_guard<std::mutex> lock(m_connMutex);

  amqp_basic_consume(m_conn, channel, queueName, amqp_empty_bytes, 0, 1, 0,
                     amqp_empty_table);

  return decodeRpcReply(amqp_get_rpc_reply(m_conn));
}

HARE_ERROR_E ConnectionBase::DeclareQueue(
    int channel, const helper::queueProperties& queueProps,
    amqp_bytes_t& retQueue) {
  const std::lock_guard<std::mutex> lock(m_connMutex);
  auto retCode = HARE_ERROR_E::ALL_GOOD;

  {
    // Should be configurable TODO
    // TODO Possible memory leak here?
    amqp_queue_declare_ok_t* r = amqp_queue_declare(
        m_conn, channel, amqp_empty_bytes, queueProps.m_passive,
        queueProps.m_durable, queueProps.m_exclusive, queueProps.m_autoDelete,
        amqp_empty_table);

    auto retCode = decodeRpcReply(amqp_get_rpc_reply(m_conn));
    if (noError(retCode)) {
      retQueue = amqp_bytes_malloc_dup(r->queue);
    } else {
      char log[80];
      sprintf(log, "Failed to declare queue on Channel: %d", channel);
      LOG(LOG_ERROR, log);
    }
    if (retQueue.bytes == NULL) {
      LOG(LOG_FATAL, "Out of memory");
    }
  }

  return retCode;
}

HARE_ERROR_E ConnectionBase::BindQueue(int channel,
                                       const amqp_bytes_t& queueName,
                                       const std::string& exchange,
                                       const std::string& bindingKey) {
  std::lock_guard<std::mutex> lock(m_connMutex);

  amqp_queue_bind(m_conn, channel, queueName,
                  amqp_cstring_bytes(exchange.c_str()),
                  amqp_cstring_bytes(bindingKey.c_str()), amqp_empty_table);

  return decodeRpcReply(amqp_get_rpc_reply(m_conn));
}

HARE_ERROR_E ConnectionBase::decodeRpcReply(amqp_rpc_reply_t reply) {
  auto retCode = HARE_ERROR_E::ALL_GOOD;

  switch (reply.reply_type) {
    case AMQP_RESPONSE_NORMAL:
      break;
    case AMQP_RESPONSE_NONE:
      LOG(LOG_WARN, "Did not receive a response from rpc_reply");
      retCode = HARE_ERROR_E::NO_RPC_REPLY;
      break;
    case AMQP_RESPONSE_LIBRARY_EXCEPTION:
      switch (reply.library_error) {
        case AMQP_STATUS_INCOMPATIBLE_AMQP_VERSION: {
          retCode = HARE_ERROR_E::INVALID_AMQP_VERSION;
          LOG(LOG_FATAL, "Invalid AMQP Version");
          break;
        }
        case AMQP_STATUS_CONNECTION_CLOSED: {
          retCode = HARE_ERROR_E::SERVER_CONNECTION_FAILURE;
          LOG(LOG_FATAL, "Connection Close Library Exception received");
          break;
        }
        case AMQP_STATUS_INVALID_PARAMETER: {
          retCode = HARE_ERROR_E::INVALID_PARAMETERS;
          LOG(LOG_ERROR, "Invalid Parameters Received by broker");
          break;
        }
        case AMQP_STATUS_SOCKET_ERROR: {
          retCode = HARE_ERROR_E::SERVER_CONNECTION_FAILURE;
          LOG(LOG_FATAL, "Socket Error received");
          break;
        }
        case AMQP_STATUS_TIMEOUT: {
          retCode = HARE_ERROR_E::TIMEOUT_OCCURED;
          LOG(LOG_DETAILED, "Timeout Occured");
          break;
        }
        case AMQP_STATUS_SOCKET_CLOSED: {
          retCode = HARE_ERROR_E::SERVER_CONNECTION_FAILURE;
          LOG(LOG_FATAL, "Socket Closed Error received");
          break;
        }
        case AMQP_STATUS_SSL_CONNECTION_FAILED: {
          retCode = HARE_ERROR_E::SERVER_AUTHENTICATION_FAILURE;
          LOG(LOG_FATAL, "SSL Connection Failure received");
          break;
        }
        case AMQP_STATUS_SSL_HOSTNAME_VERIFY_FAILED: {
          retCode = HARE_ERROR_E::SERVER_AUTHENTICATION_FAILURE;
          LOG(LOG_FATAL, "SSL Hostname Verification Failure received");
          break;
        }
        case AMQP_STATUS_UNEXPECTED_STATE: {
          retCode = HARE_ERROR_E::SERVER_CONNECTION_FAILURE;
          LOG(LOG_FATAL, "Server Connection Failure");
          break;
        }
        default:
          LOG(LOG_ERROR, "HareCpp doesn't have this error known");
          printf("%d\n", reply.library_error);
          break;
      }
      break;
    case AMQP_RESPONSE_SERVER_EXCEPTION:
      switch (reply.reply.id) {
        case AMQP_CONNECTION_CLOSE_METHOD: {
          retCode = HARE_ERROR_E::SERVER_CONNECTION_FAILURE;
          LOG(LOG_FATAL, "Connection Close Exception received");
          break;
        }
        case AMQP_CHANNEL_CLOSE_METHOD: {
          retCode = HARE_ERROR_E::CHANNEL_EXCEPTION;
          LOG(LOG_ERROR, "Channel Exception received");
          break;
        }
        default: {
          LOG(LOG_ERROR, "Unknown Server Exception received");
          break;
        }
      }
  }
  return retCode;
}

}  // namespace connection
}  // namespace HareCpp