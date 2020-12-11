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

#include "Utils.hpp"
#include "pch.hpp"

namespace HareCpp {

class Message {
 private:
  amqp_bytes_t m_body;
  bool m_bodyHasBeenSet;

  amqp_basic_properties_t m_properties;

 public:
  /**
   * Constructor declarations
   */
  explicit Message() : m_bodyHasBeenSet{false} { m_properties._flags = 0; };
  explicit Message(std::string&& message);
  explicit Message(const std::string& message);
  explicit Message(const amqp_envelope_t& envelope);

  /**
   * Copy Constructor
   *
   * Needs the logic to do full copy of properties and message body.
   */
  Message(const Message& copiedFrom);

  Message& operator=(const Message&) = default;

  amqp_basic_properties_t* AmqpProperties() { return &m_properties; }

  /**
   *  String()
   *
   *  Returns message payload to std::string as copy.  Will return a empty
   * string if body has not been set
   */
  std::string String() const;

  /**
   * Payload()
   *
   * Returns a char* to the payload, or nullptr if body wasn't set
   */
  const char* Payload() const;

  /**
   * Bytes()
   *
   * Returns a pointer to the amqp_bytes_t used as the payload.
   * This contains a len (length) and bytes (body) as void*
   */
  const amqp_bytes_t* Bytes() const;

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

  /**
   * setReplyTo
   */
  void SetReplyTo(const std::string& replyTo);

  void SetReplyTo(const char*& replyTo);

  const std::string ReplyTo();

  bool ReplyToIsSet() const;

  /**
   * SetPayload - using char*
   *
   * @param [in] payload: const char* of the payload.  Requires \0 at the end
   * @return void
   */
  void SetPayload(const char* payload);

  /**
   * SetPayload - using void*
   *
   * @param [in] payload: void* of the payload
   * @param [in] size: size of the string of bytes pointed to by the payload
   * pointer
   * @return void
   */
  void SetPayload(void* payload, const int size);

  bool TimestampIsSet() const;

  /**
   * Assumed Microseconds
   */
  void SetTimestamp(uint64_t timestamp);

  uint64_t Timestamp() const;

  /**
   * setCorrelationId
   */
  void SetCorrelationId(const char*& correlationId);

  void SetCorrelationId(const std::string& correlationId);

  bool HasCorrelationId();

  /**
   *  Default Destructor
   */
  ~Message() {
    if (m_bodyHasBeenSet) free(m_body.bytes);
  }
};

}  // Namespace HareCpp

#endif  // _MESSAGE_H_