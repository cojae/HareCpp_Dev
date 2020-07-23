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

#ifndef _CHANNEL_HANDLER_H_
#define _CHANNEL_HANDLER_H_

#include "HelperStructs.hpp"
#include "Message.hpp"
#include "pch.hpp"

#include <map>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

namespace HareCpp {

/**
 * ChannelHandler is a helper class to keep track of channel/callback information for consumers
 * It gives (I hope) an easier way to access and set channel related information
 */
class ChannelHandler {
 private:
  // Name to be changed
  struct channelProcessingInfo {
    std::shared_ptr<int> m_channel;
    std::shared_ptr<const std::pair<std::string, std::string> > m_bindingPair;
    TD_Callback m_callback;

    amqp_bytes_t m_queueName;
    // Stored so we can access them again if channel not accessible at time of
    // creation
    helper::queueProperties m_queueProperties;
  };

  /**
   * Lookup structure to find a particular binding pair.  The binding pair is the 
   * combination of exchange and routing key, and this combination is used with 
   * the callback set by the user to subscribe to and process desired messages.
   * 
   * The map is keyed off the pair itself with the content being a shared ptr to the channelProcessingInfo structure,
   * This is so that the channelLookup and the bindingPairLookup can have shared structures, so to reduce redundant memory usage.
   */
  std::map<const std::pair<std::string, std::string>,
           std::shared_ptr<channelProcessingInfo> >
      m_bindingPairLookup;

  /**
   * Lookup structure to find a particular channel.  This channel is bound directly 
   * to a binding Pair (see bindingPairLookup comments).  Often we only have the channel, it helps with this structure to easily find the
   * channel information given only a channel integer.
   * 
   * This shares the same channelProcessingInfo as m_bindingPairLookup, for easy access/quicker lookup
   */
  std::map<int, std::shared_ptr<channelProcessingInfo> > m_channelLookup;

  // Keeps track of channels, starts at 0 and increments with every new addition to the channelHandler
  int m_nextAvailableChannel;

  /**
   *  Determines if the processing of the consumed messages should be multi-threaded or not.  
   *   If multi-threaded is turned off, you reduce the copies but you have to maintain processing speed yourself.
   *   If multi-threaded is turned on, a lambda is produced to copy the callback and the message over to its own space.
   *   Default is turned 'on'
   */
  bool m_multiThreaded;

  // Mutex to protect the class members
  mutable std::mutex m_handlerMutex ;

 public:
  ChannelHandler();

  ~ChannelHandler() {
    // Free everything to avoid memory leak
    for (auto it : m_channelLookup) {
      free(it.second->m_queueName.bytes);
    }
  };

  // TODO These need return codes
  // TODO user of this class must use make_pair, so maybe remove pair

  /**
   * Adds a channel processing group to the current class instances list
   * This includes the addition of callback function, and the associated channel/binding pairs.
   * This is to be later used while processing a message, the message will contain the exchange/binding key used
   * and that combination will be used to lookup the associated callback function to call to process the message
   * 
   * @param [in] bindingPair : The pair of exchange name and routing key
   * @param [in] callback : The callback function used by the user to process this message
   * @returns integer value for its success (-1 = fails) TODO use HARE_ERROR_E
   */
  int addChannelProcessor(
      const std::pair<std::string, std::string>& bindingPair,
      TD_Callback& callback);

  /**
   * Removes a channel processing group from the current class instances list
   * This includes the removal of callback function, and the associated channel/binding pairs
   * 
   * @param [in] bindingPair : The pair of exchange name and routing key
   * @returns integer value for its success (-1 = fails) TODO use HARE_ERROR_E
   */
  int removeChannelProcessor(
      const std::pair<std::string, std::string>& bindingPair);

  /**
   * set the multiThreaded boolean (default true/on)
   * 
   * @param [in] multiThread: boolean to set on/off (true/false) for whether or not we are processing with multible threads
   */
  void setMultiThreaded(bool multiThread);

  /**
   * Process the message we have been passed by the consumer.  ChannelHandler keeps track of channel information, including the user's callback function.  
   * So messages can be passed to it for processing
   * 
   * By default, this is multithreaded.  But can be turned down to allow one message at a time (setMultiThreaded(false);)
   * 
   * @param [in] bindingPair: the pair of exchange/routingKey used to determine where the message came from and what callback we care about
   * @param [in] message: The message to be processed
   */
  void process(const std::pair<std::string, std::string>& bindingPair,
               const Message& message);

  /**
   * Returns a vector of all channels
   * 
   * This is beneficial when trying to establish connection with broker
   * 
   * @returns vector of channels that are/will be subscribed to
   */
  std::vector<int> getChannelList() const;

  /**
   * Get the exchange name given a channel number
   * 
   * @param [in] channel: the channel we are looking up
   * @returns the exchange name as string
   */
  std::string getExchange(int channel) {
    std::lock_guard<std::mutex> lock(m_handlerMutex);
    if (m_channelLookup.count(channel) != 0)
      return m_channelLookup[channel]->m_bindingPair->first;
    else
      return "";  // Should error TODO
  }

  /**
   * Get the binding Key (Routing Key) given a channel number
   * 
   * @param [in] channel: the channel we are looking up
   * @returns the binding/routing key for the messages
   */
  std::string getBindingKey(int channel) {
    std::lock_guard<std::mutex> lock(m_handlerMutex);
    if (m_channelLookup.count(channel) != 0)
      return m_channelLookup[channel]->m_bindingPair->second;
    else
      return "";  // Should error
  }

  /**
   * Get the queue name given a channel number
   * 
   * @param [in] channel: the channel we are looking up
   * @returns the amqp_bytes_t of the queue, to be used when subscribing to broker exchange/channel
   */
  amqp_bytes_t getQueueName(int channel) {
    std::lock_guard<std::mutex> lock(m_handlerMutex);
    if (m_channelLookup.count(channel) != 0)
      return m_channelLookup[channel]->m_queueName;
    else
      return amqp_empty_bytes;  // Should error
  }

  /**
   * Set the queue name with a channel number and the associated queue name (which may be overwriten/updated later)
   * 
   * @param [in] channel: the channel to be used at lookup (getter functions)
   * @param [in] queueName: the amqp_bytes_t of the queue to be used to set within the lookup maps
   * @returns nothing - TODO return HARE_ERROR_E
   */
  void setQueueName(int channel, const amqp_bytes_t& queueName) {
    std::lock_guard<std::mutex> lock(m_handlerMutex);
    if (m_channelLookup.count(channel) != 0) {
      amqp_bytes_free(m_channelLookup[channel]->m_queueName);
      m_channelLookup[channel]->m_queueName = queueName;
    } else {
      return; /* TODO error */
    }
    return;
  }

  /**
   * Get the queue properties given a channel number
   * 
   * @param [in] channel: the channel we are looking up
   * @returns the queueProperties to be used when subscribing to an exchange within a broker 
   */
  helper::queueProperties getQueueProperties(int channel) {
    std::lock_guard<std::mutex> lock(m_handlerMutex);
    if (m_channelLookup.count(channel) != 0)
      return m_channelLookup[channel]->m_queueProperties;
    else
      return helper::queueProperties();
  }

  /**
   * Set the queue properties given a channel number
   * 
   * @param [in] channel: the channel we will be looking up
   * @param [in] queueProperties: the properties set for that channel/queue
   * @returns nothing - TODO HARE_ERROR_E
   */
  void setQueueProperties(int channel,
                          const helper::queueProperties& queueProperties) {
    std::lock_guard<std::mutex> lock(m_handlerMutex);
    if (m_channelLookup.count(channel) != 0) {
      m_channelLookup[channel]->m_queueProperties = queueProperties;
    } else {
      return; /* TODO error */
    }
    return;
  }
};

}  // Namespace HareCpp

#endif /*CHANNEL_HANDLER_H*/