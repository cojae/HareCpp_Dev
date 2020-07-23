#include "pch.hpp"

#ifndef _HELPER_STRUCTS_H_
#define _HELPER_STRUCTS_H_

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

// TODO instead of this AND Message class, try and find a middleground between the two
// Redundant stuff is annoying. This is redundant
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