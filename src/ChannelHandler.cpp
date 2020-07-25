#include "ChannelHandler.hpp"

namespace HareCpp {

ChannelHandler::ChannelHandler()
    : m_nextAvailableChannel(1), m_multiThreaded(true) {}

int ChannelHandler::addChannelProcessor(
    const std::pair<std::string, std::string>& bindingPair,
    TD_Callback& callback) {
  int retCode = -1;
  std::lock_guard<std::mutex> lock(m_handlerMutex);
  // Check that we don't already have it

  auto it = m_bindingPairLookup.find(bindingPair);

  if (it != m_bindingPairLookup.end()) {
    char log[80];
    // TODO sprintf is not safe, please fix all calls
    sprintf(log, "%s : %s already exists, updating callback",
            bindingPair.first.c_str(), bindingPair.second.c_str());
    LOG(LOG_WARN, log);
    // Set new callback
    it->second->m_callback = callback;
  } else /* New Pairing */
  {
    char log[80];
    sprintf(log, "%s : %s doesn't exist, creating in map",
            bindingPair.first.c_str(), bindingPair.second.c_str());
    LOG(LOG_DETAILED, log);

    /**
     *  I did this complicated way as a means to make lookups faster
     *  But this section of code may have broke it  TODO
     */
    m_channelLookup[m_nextAvailableChannel] =
        std::make_shared<channelProcessingInfo>();
    m_bindingPairLookup.insert(
        std::make_pair(bindingPair, m_channelLookup[m_nextAvailableChannel]));

    it = m_bindingPairLookup.find(bindingPair);

    m_channelLookup[m_nextAvailableChannel]->m_callback = callback;
    m_channelLookup[m_nextAvailableChannel]->m_bindingPair =
        std::make_shared<const std::pair<std::string, std::string>>(it->first);

    retCode = m_nextAvailableChannel;
    m_nextAvailableChannel++;
  }
  return retCode;
}

int ChannelHandler::removeChannelProcessor(
    const std::pair<std::string, std::string>& bindingPair) {
  auto it = m_bindingPairLookup.find(bindingPair);
  if (it != m_bindingPairLookup.end()) {
    m_channelLookup.erase(*it->second->m_channel);
    m_bindingPairLookup.erase(it);
  } else {
    return -1;
  }
  return 1;
}

void ChannelHandler::setMultiThreaded(bool multiThread) {
  std::lock_guard<std::mutex> lock(m_handlerMutex);
  m_multiThreaded = multiThread;
}

void ChannelHandler::process(
    const std::pair<std::string, std::string>& bindingPair,
    const Message& message) {
  std::lock_guard<std::mutex> lock(m_handlerMutex);
  auto it = m_bindingPairLookup.find(bindingPair);
  if (it == m_bindingPairLookup.end()) {
    return;  // Error
  }

  if (m_multiThreaded) {
    std::thread([message, it] { it->second->m_callback(message); }).detach();
  } else {
    it->second->m_callback(message);
  }
}

std::vector<int> ChannelHandler::getChannelList() const {
  std::vector<int> retVec;
  std::lock_guard<std::mutex> lock(m_handlerMutex);
  if (m_channelLookup.size() == 0) return retVec;  // error code here
  for (auto const& it : m_channelLookup) {
    retVec.push_back(it.first);
  }
  return retVec;
}

}  // namespace HareCpp