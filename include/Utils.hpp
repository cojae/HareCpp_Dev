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

#ifndef _HARE_UTILS_HPP_
#define _HARE_UTILS_HPP_

#include "cstring"
#include "pch.hpp"

namespace HareCpp {
/**
 *  This is because using the default rabbitmq-c library calls
 *  is a little harder to follow :
 * (amqp_bytes_malloc_dup(amqp_cstring_bytes(str)))
 */
inline amqp_bytes_t hare_cstring_bytes(const char *str) {
  amqp_bytes_t result;
  result.len = strlen(str);
  result.bytes = malloc(result.len + 1);
  if (result.bytes != nullptr) {
    memcpy(result.bytes, (void *)str, result.len + 1);
  }
  return result;
};

inline amqp_bytes_t hare_void_bytes(const void *data, const int size) {
  amqp_bytes_t result;
  result.len = size;
  result.bytes = malloc(size);
  if (result.bytes != nullptr) {
    memcpy(result.bytes, data, size);
  }
  return result;
}

/**
 * Turns amqp_bytes_t into a string (via static cast and creation of
 * std::string)
 */
inline std::string hare_bytes_to_string(amqp_bytes_t bytes) {
  return std::string(static_cast<char *>(bytes.bytes), bytes.len);
}

/**
 * Makes a full clone of amqp_basic_properties_t.  This is done similarly (and
 * using) amqp_bytes_malloc_dup, which information can be found on the rabbitmqc
 * documentation/repo.
 *
 * This is to be used when copying, to avoid a free causing data to go missing.
 *
 * @param [in] properties : the properties that will be cloned (full malloc of
 * resources)
 * @param [out] clonedProperties :  acting as the full clone of the original
 * properties
 */
inline void hare_basic_properties_malloc_dup(
    const amqp_basic_properties_t &properties,
    amqp_basic_properties_t &clonedProperties) {
  clonedProperties._flags = properties._flags;
  if (AMQP_BASIC_CONTENT_TYPE_FLAG ==
      (properties._flags & AMQP_BASIC_CONTENT_TYPE_FLAG))
    clonedProperties.content_type =
        amqp_bytes_malloc_dup(properties.content_type);
  if (AMQP_BASIC_CONTENT_ENCODING_FLAG ==
      (properties._flags & AMQP_BASIC_CONTENT_ENCODING_FLAG))
    clonedProperties.content_encoding =
        amqp_bytes_malloc_dup(properties.content_encoding);
  clonedProperties.headers = properties.headers;  // TODO may be incorrect
  clonedProperties.delivery_mode = properties.delivery_mode;
  clonedProperties.priority = properties.priority;
  if (AMQP_BASIC_CORRELATION_ID_FLAG ==
      (properties._flags & AMQP_BASIC_CORRELATION_ID_FLAG))
    clonedProperties.correlation_id =
        amqp_bytes_malloc_dup(properties.correlation_id);
  if (AMQP_BASIC_REPLY_TO_FLAG ==
      (properties._flags & AMQP_BASIC_REPLY_TO_FLAG))
    clonedProperties.reply_to = amqp_bytes_malloc_dup(properties.reply_to);
  if (AMQP_BASIC_EXPIRATION_FLAG ==
      (properties._flags & AMQP_BASIC_EXPIRATION_FLAG))
    clonedProperties.expiration = amqp_bytes_malloc_dup(properties.expiration);
  if (AMQP_BASIC_MESSAGE_ID_FLAG ==
      (properties._flags & AMQP_BASIC_MESSAGE_ID_FLAG))
    clonedProperties.message_id = amqp_bytes_malloc_dup(properties.message_id);
  clonedProperties.timestamp = properties.timestamp;
  if (AMQP_BASIC_TYPE_FLAG == (properties._flags & AMQP_BASIC_TYPE_FLAG))
    clonedProperties.type = amqp_bytes_malloc_dup(properties.type);
  if (AMQP_BASIC_USER_ID_FLAG == (properties._flags & AMQP_BASIC_USER_ID_FLAG))
    clonedProperties.user_id = amqp_bytes_malloc_dup(properties.user_id);
  if (AMQP_BASIC_APP_ID_FLAG == (properties._flags & AMQP_BASIC_APP_ID_FLAG))
    clonedProperties.app_id = amqp_bytes_malloc_dup(properties.app_id);
  if (AMQP_BASIC_CLUSTER_ID_FLAG ==
      (properties._flags & AMQP_BASIC_CLUSTER_ID_FLAG))
    clonedProperties.cluster_id = amqp_bytes_malloc_dup(properties.cluster_id);
}

/**
 * Turn rpc reply into a char* exception string.  This is convenient for
 * logging.
 *
 * NOTE: May be unused, as connectionBase seems to do this fine
 */
inline const char *amqp_server_exception_string(amqp_rpc_reply_t r) {
  int res;
  static char s[512];

  switch (r.reply.id) {
    case AMQP_CONNECTION_CLOSE_METHOD: {
      amqp_connection_close_t *m = (amqp_connection_close_t *)r.reply.decoded;
      res = snprintf(s, sizeof(s), "server connection error %d, message: %.*s",
                     m->reply_code, (int)m->reply_text.len,
                     (char *)m->reply_text.bytes);
      break;
    }

    case AMQP_CHANNEL_CLOSE_METHOD: {
      amqp_channel_close_t *m = (amqp_channel_close_t *)r.reply.decoded;
      res = snprintf(s, sizeof(s), "server channel error %d, message: %.*s",
                     m->reply_code, (int)m->reply_text.len,
                     (char *)m->reply_text.bytes);
      break;
    }

    default:
      res = snprintf(s, sizeof(s), "unknown server error, method id 0x%08X",
                     r.reply.id);
      break;
  }

  return res >= 0 ? s : NULL;
}

/**
 * General debug function, used for logging out unknown rpc replies
 */
inline const char *amqp_rpc_reply_string(amqp_rpc_reply_t r) {
  switch (r.reply_type) {
    case AMQP_RESPONSE_NORMAL:
      return "normal response";
    case AMQP_RESPONSE_NONE:
      return "missing RPC reply Type";
    case AMQP_RESPONSE_LIBRARY_EXCEPTION:
      return amqp_error_string2(r.library_error);
    case AMQP_RESPONSE_SERVER_EXCEPTION:
      return amqp_server_exception_string(r);
    default:
      return "";
  }
}
}  // namespace HareCpp

#endif  // __HARE_UTILS_HPP_