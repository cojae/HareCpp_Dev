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
#include "pch.hpp"

#ifndef _HELPER_STRUCTS_H_
#define _HELPER_STRUCTS_H_

// TODO i don't like this namespace word choice/combination...
namespace HareCpp {
namespace helper {

struct queueProperties {
  queueProperties()
      : m_passive(0), m_durable(0), m_exclusive(0), m_autoDelete(1){};
  int m_passive;
  int m_durable;
  int m_exclusive;
  int m_autoDelete;
};

struct RawMessage {
  amqp_bytes_t exchange;
  int channel;
  amqp_bytes_t routing_value;
  amqp_basic_properties_t properties;
  amqp_bytes_t message;
};

struct loginCredentials {
  loginCredentials(const std::string& hostname, int port,
                   const std::string& username, const std::string& password)
      : m_hostname(hostname),
        m_port(port),
        m_username(username),
        m_password(password){};
  std::string m_hostname;
  int m_port;
  std::string m_username;
  std::string m_password;
};

struct sslCredentials {
  std::string m_pathToCACert;
  std::string m_pathToClientKey;
  std::string m_pathToClientCert;
};
}  // namespace helper
}  // namespace HareCpp

#endif  // _HELPER_STRUCTS_H_