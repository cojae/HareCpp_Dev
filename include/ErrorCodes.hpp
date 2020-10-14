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
   * Unable to close channel
   */
  UNABLE_TO_CLOSE_CHANNEL,
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
