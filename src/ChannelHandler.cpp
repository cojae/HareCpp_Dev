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
#include "ChannelHandler.hpp"

namespace HareCpp {

ChannelHandler::ChannelHandler()
    : m_nextAvailableChannel(1), m_multiThreaded(false) {}

int ChannelHandler::AddChannelProcessor(const HashableBindingPair& bindingPair,
                                        TD_Callback& callback) {
  auto retCode{-1};
  std::lock_guard<std::mutex> lock(m_handlerMutex);

  // Check that we don't already have it
  auto it{m_bindingPairLookup.find(bindingPair)};

  if (it != m_bindingPairLookup.end()) {
    char log[LOG_MAX_CHAR_SIZE];
    snprintf(
        log, LOG_MAX_CHAR_SIZE, "%s : %s already exists, updating callback",
        bindingPair.m_exchangeName.c_str(), bindingPair.m_routingKey.c_str());
    LOG(LOG_WARN, log);
    // Set new callback
    it->second->m_callback = callback;
  } else /* New Pairing */
  {
    char log[LOG_MAX_CHAR_SIZE];
    snprintf(log, LOG_MAX_CHAR_SIZE, "%s : %s doesn't exist, creating in map",
             bindingPair.m_exchangeName.c_str(),
             bindingPair.m_routingKey.c_str());
    LOG(LOG_DETAILED, log);

    m_channelLookup[m_nextAvailableChannel] =
        std::make_shared<channelProcessingInfo>();
    m_bindingPairLookup.insert(
        std::make_pair(bindingPair, m_channelLookup[m_nextAvailableChannel]));


    m_channelLookup[m_nextAvailableChannel]->m_callback = callback;

    it = m_bindingPairLookup.find(bindingPair);
    m_channelLookup[m_nextAvailableChannel]->m_bindingPair =
      std::make_shared<HashableBindingPair>(it->first);

    retCode = m_nextAvailableChannel;
    m_nextAvailableChannel++;
  }
  return retCode;
}

int ChannelHandler::RemoveChannelProcessor(
    const HashableBindingPair& bindingPair) {
  auto it{m_bindingPairLookup.find(bindingPair)};
  if (it != m_bindingPairLookup.end()) {
    m_channelLookup.erase(*it->second->m_channel);
    m_bindingPairLookup.erase(it);
  } else {
    return -1;
  }
  return 1;
}

void ChannelHandler::SetMultiThreaded(bool multiThread) {
  std::lock_guard<std::mutex> lock(m_handlerMutex);
  m_multiThreaded = multiThread;
}

void ChannelHandler::Process(const HashableBindingPair& bindingPair,
                             const Message& message) {
  /* Log receipt of processing */
  {
    char log[LOG_MAX_CHAR_SIZE];
    snprintf(log, LOG_MAX_CHAR_SIZE, "Processing message from %s : %s",
             bindingPair.m_exchangeName.c_str(),
             bindingPair.m_routingKey.c_str());
    LOG(LOG_DETAILED, log);
  }

  std::lock_guard<std::mutex> lock(m_handlerMutex);
  auto it{m_bindingPairLookup.find(bindingPair)};
  if (it == m_bindingPairLookup.end() || it->second == nullptr) {
    return;  // Error
  }

  if (m_multiThreaded) {
    // This makes a copy of the function, in order to avoid race condition
    // as m_handlerMutex doesn't follow to this thread
    std::thread callbackThread([message](TD_Callback func) { func(message); },
                               it->second->m_callback);
    callbackThread.detach();
  } else {
    it->second->m_callback(message);
  }
}

std::vector<int> ChannelHandler::GetChannelList() const {
  std::vector<int> retVec;
  std::lock_guard<std::mutex> lock(m_handlerMutex);
  if (m_channelLookup.size() == 0) return retVec;  // error code here
  for (auto const& it : m_channelLookup) {
    retVec.push_back(it.first);
  }
  return retVec;
}

}  // namespace HareCpp