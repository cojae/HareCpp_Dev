#include "Message.hpp"

namespace HareCpp {

Message::Message(std::string&& message) : m_message(std::move(message)) {
  m_properties._flags = 0;
}

Message::Message(const std::string& message) : m_message(message) {
  m_properties._flags = 0;
}

/**
 * Message created by receiving an envelope
 */
Message::Message(const amqp_envelope_t& envelope) 
    : m_message(std::string(static_cast<char*>(envelope.message.body.bytes),envelope.message.body.len)),
    m_properties(envelope.message.properties) {}

const std::string* Message::payload() const {
  /*if (m_message_body.bytes == NULL) {
    // TODO set errorCode
  }*/

  return &m_message;
  /*
  return std::string(
      (char*)m_message_body.bytes,m_message_body.len
    );
    */
}

/*
const amqp_bytes_t* Message::getAmqpBytes() const {
  return envelope.message.body ;
}*/

unsigned int Message::Length() const { return m_message.size(); }
}  // namespace HareCpp
