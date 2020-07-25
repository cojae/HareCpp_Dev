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

/**
 *  This is because using the default rabbitmq-c library requires too much
 * processing this should be faster
 */
inline amqp_bytes_t hare_cstring_bytes(const char *str) {
  amqp_bytes_t result;
  result.len = strlen(str);
  result.bytes = malloc(result.len);
  if (result.bytes != nullptr) {
    memcpy(result.bytes, (void *)str, result.len);
  }
  return result;
};

inline std::string hare_bytes_to_string(amqp_bytes_t bytes) {
  return std::string(static_cast<char *>(bytes.bytes), bytes.len);
}

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

#endif  // __HARE_UTILS_HPP_