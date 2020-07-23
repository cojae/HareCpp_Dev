#ifndef _HARE_ERROR_H_
#define _HARE_ERROR_H_

namespace HareCpp {

enum class HARE_ERROR_E : unsigned int {
  /**
   * No errors
   */
  ALL_GOOD,
  /**
   *
   */
  INVALID_AMQP_VERSION,
  /**
   *
   */
  TIMEOUT_OCCURED,
  /**
   *
   */
  PUBLISH_ERROR,
  /**
   * The class has not been initialized and cannot be used
   */
  NOT_INITIALIZED,
  /**
   * General inability to initialize
   */
  INITIALIZE_FAILURE,
  /**
   * The Initialize parameters were invalid (couldn't build config)
   */
  INVALID_PARAMETERS,
  /**
   * Could not start Producer
   */
  PRODUCER_CREATION_FAILED,
  /**
   * Could not start Consumer
   */
  CONSUMER_CREATION_FAILED,
  /**
   * Thread is already running (cannot start another)
   */
  THREAD_ALREADY_RUNNING,
  /**
   * Thread is not running
   */
  THREAD_NOT_RUNNING,
  /**
   * Cannot connect to server
   */
  SERVER_CONNECTION_FAILURE,
  /**
   * Authentication to server failure
   */
  SERVER_AUTHENTICATION_FAILURE,
  /**
   * Server replied with exception
   */
  SERVER_EXCEPTION_RESPONSE,
  /**
   * Unable to subscribe to topic(s)
   */
  UNABLE_TO_SUBSCRIBE,
  /**
   * 
   */
  PRODUCER_QUEUE_FULL,
  /**
   * Unable to open channel
   */
  UNABLE_TO_OPEN_CHANNEL,
  /**
   * General Channel Exception. Must restart it
   */
  CHANNEL_EXCEPTION,
  /**
   * No RPC Reply from connection action
   */
  NO_RPC_REPLY,
};

inline bool noError(HARE_ERROR_E retCode) {
  return (HARE_ERROR_E::ALL_GOOD == retCode);
};

inline bool serverFailure(HARE_ERROR_E retCode) {
  return (retCode == HARE_ERROR_E::SERVER_CONNECTION_FAILURE ||
          retCode == HARE_ERROR_E::SERVER_EXCEPTION_RESPONSE);
}

};  // Namespace HareCpp

#endif /* HARE_ERROR_H */
