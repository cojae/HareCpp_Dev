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

#ifndef _HASH_BINDING_PAIR_H_
#define _HASH_BINDING_PAIR_H_

#include <string>

namespace HareCpp {

/**
 * HashableBindingPair
 *
 * Struct created to hold associated exchange/routing keys for consumer to
 * subscribe to and call upon receipt of receiving (callback). This needs to be
 * made into a specific struct so that we can hash it for potentially O(1)
 * lookup when used with ChannelHandler's m_bindingPairLookup hashmap.
 */
struct HashableBindingPair {
  std::string m_exchangeName;
  std::string m_routingKey;

  bool operator==(const HashableBindingPair& other) const {
    return (m_exchangeName == other.m_exchangeName &&
            m_routingKey == other.m_routingKey);
  }
};

}  // namespace HareCpp

namespace std {

template <>
struct hash<HareCpp::HashableBindingPair> {
  std::size_t operator()(const HareCpp::HashableBindingPair& hbp) const {
    return ((std::hash<std::string>()(hbp.m_exchangeName) ^
             (std::hash<std::string>()(hbp.m_routingKey) << 1)) >>
            1);
  }
};

}  // namespace std
#endif /* HASH_BINDING_PAIR_H_ */