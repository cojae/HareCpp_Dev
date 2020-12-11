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
#include "Message.hpp"

namespace HareCpp {

Message::Message(std::string&& message)
    : m_body{hare_cstring_bytes(message.c_str())} {
  m_bodyHasBeenSet = true;
  m_properties._flags = 0;
}

Message::Message(const std::string& message)
    : m_body{hare_cstring_bytes(message.c_str())} {
  m_bodyHasBeenSet = true;
  m_properties._flags = 0;
}

/**
 * Message created by receiving an envelope
 */
Message::Message(const amqp_envelope_t& envelope) {
  m_bodyHasBeenSet = true;
  m_body = amqp_bytes_malloc_dup(envelope.message.body);
  hare_basic_properties_malloc_dup(envelope.message.properties, m_properties);
}

Message::Message(const Message& copiedFrom) {
  hare_basic_properties_malloc_dup(copiedFrom.m_properties, m_properties);
  m_bodyHasBeenSet = copiedFrom.m_bodyHasBeenSet;
  m_body = amqp_bytes_malloc_dup(copiedFrom.m_body);
}

std::string Message::String() const {
  return (m_bodyHasBeenSet
              ? std::string(static_cast<char*>(m_body.bytes), m_body.len)
              : std::string(""));
}

const char* Message::Payload() const {
  return (m_bodyHasBeenSet ? static_cast<char*>(m_body.bytes) : nullptr);
}

const amqp_bytes_t* Message::Bytes() const { return &m_body; }

unsigned int Message::Length() const {
  if (m_bodyHasBeenSet)
    return m_body.len;
  else
    return 0;
}

void Message::SetReplyTo(const std::string& replyTo) {
  m_properties._flags |= AMQP_BASIC_REPLY_TO_FLAG;
  m_properties.reply_to = amqp_cstring_bytes(replyTo.c_str());
}

void Message::SetReplyTo(const char*& replyTo) {
  m_properties._flags |= AMQP_BASIC_REPLY_TO_FLAG;
  m_properties.reply_to = amqp_cstring_bytes(replyTo);
}

const std::string Message::ReplyTo() {
  if (ReplyToIsSet()) {
    return hare_bytes_to_string(m_properties.reply_to);
  } else {
    return "";
  }
}

bool Message::ReplyToIsSet() const {
  return (AMQP_BASIC_REPLY_TO_FLAG ==
          (m_properties._flags & AMQP_BASIC_REPLY_TO_FLAG));
}

void Message::SetPayload(const char* payload) {
  m_bodyHasBeenSet = true;
  m_body = hare_cstring_bytes(payload);
}

void Message::SetPayload(void* payload, const int size) {
  m_bodyHasBeenSet = true;
  m_body = hare_void_bytes(payload, size);
}

bool Message::TimestampIsSet() const {
  return (AMQP_BASIC_TIMESTAMP_FLAG ==
          (m_properties._flags & AMQP_BASIC_TIMESTAMP_FLAG));
}

void Message::SetTimestamp(uint64_t timestamp) {
  m_properties._flags |= AMQP_BASIC_TIMESTAMP_FLAG;
  m_properties.timestamp = timestamp;
}

uint64_t Message::Timestamp() const {
  uint64_t retVal = 0;
  if (TimestampIsSet()) {
    retVal = m_properties.timestamp;
  }
  return retVal;
}
void Message::SetCorrelationId(const char*& correlationId) {
  m_properties._flags |= AMQP_BASIC_CORRELATION_ID_FLAG;
  m_properties.correlation_id = amqp_cstring_bytes(correlationId);
};

void Message::SetCorrelationId(const std::string& correlationId) {
  m_properties._flags |= AMQP_BASIC_CORRELATION_ID_FLAG;
  m_properties.correlation_id = amqp_cstring_bytes(correlationId.c_str());
};

bool Message::HasCorrelationId() {
  return (AMQP_BASIC_CORRELATION_ID_FLAG ==
          (m_properties._flags & AMQP_BASIC_CORRELATION_ID_FLAG));
}

}  // namespace HareCpp
