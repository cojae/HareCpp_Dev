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

#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include "pch.hpp"

namespace HareCpp {

// TODO this class may be completely tossed and replaced with HelperStructs::RawMessage
class Message {
 private:
  /**
   * AMQP Message as string
   */
  std::string m_message;

  amqp_basic_properties_t m_properties;

 public:
  /**
   * Constructor declarations
   */
  Message() { m_properties._flags = 0; };
  Message(std::string&& message);
  Message(const std::string& message);
  Message(const amqp_envelope_t& envelope);

  Message(const Message&) = default;

  Message(Message&& original) 
    : m_message(std::move(original.m_message)) {
    // This is a pure copy.  Can potentially make this faster
    m_properties = original.m_properties;
    original.m_message = "";
  };

  Message& operator=(const Message&) = default;

  amqp_basic_properties_t* getAmqpProperties() { return &m_properties; }

  /**
   *  payload()
   *
   *  Returns message payload to std::string
   */
  const std::string* payload() const;

  /**
   * Length()
   *
   * Returns length of the payload
   */
  unsigned int Length() const;

  /**
   * setDeliveryMode
   *
   * AMQP_DELIVERY_NONPERSISTENT
   * AMQP_DELIVERY_PERSISTENT
   *
   * TODO Wrap these enums in a Hare specific one that includes them
   */
  void setDeliveryMode(int deliveryMode);

  amqp_bytes_t getAmqpBytes();

  /**
   * setReplyTo
   */
  void setReplyTo(const std::string& replyTo) {
    m_properties._flags |= AMQP_BASIC_REPLY_TO_FLAG;
    m_properties.reply_to = amqp_cstring_bytes(replyTo.c_str());
  };

  void setReplyTo(const char*& replyTo) {
    m_properties._flags |= AMQP_BASIC_REPLY_TO_FLAG;
    m_properties.reply_to = amqp_cstring_bytes(replyTo);
  };

  bool replyToIsSet() const {
    return (AMQP_BASIC_REPLY_TO_FLAG ==
            (m_properties._flags & AMQP_BASIC_REPLY_TO_FLAG));
  };

  bool timestampIsSet() const {
    return (AMQP_BASIC_TIMESTAMP_FLAG ==
            (m_properties._flags & AMQP_BASIC_TIMESTAMP_FLAG));
  }

  /**
   * Assumed Microseconds
   */
  void setTimestamp(uint64_t timestamp) {
    m_properties._flags |= AMQP_BASIC_TIMESTAMP_FLAG;
    m_properties.timestamp = timestamp;
  }

  uint64_t getTimestamp() const {
    uint64_t retVal = 0;
    if (timestampIsSet()) {
      retVal = m_properties.timestamp;
    }
    return retVal;
  }

  /**
   * setCorrelationId
   */
  void setCorrelationId(const char*& correlationId) {
    m_properties._flags |= AMQP_BASIC_CORRELATION_ID_FLAG;
    m_properties.correlation_id = amqp_cstring_bytes(correlationId);
  };

  void setCorrelationId(const std::string& correlationId) {
    m_properties._flags |= AMQP_BASIC_CORRELATION_ID_FLAG;
    m_properties.correlation_id = amqp_cstring_bytes(correlationId.c_str());
  };

  /**
   *  Default Destructor
   */
  ~Message(){};
};

}  // Namespace HareCpp

#endif  // _MESSAGE_H_