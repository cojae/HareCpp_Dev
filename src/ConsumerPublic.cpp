#include "Consumer.hpp"
#include "Utils.hpp"

namespace HareCpp {

HARE_ERROR_E Consumer::subscribe(const std::string& exchange,
                                 const std::string& binding_key, TD_Callback f,
                                 helper::queueProperties queueProps) {
  std::lock_guard<std::mutex> lock(m_consumerMutex);
  auto retCode = HARE_ERROR_E::ALL_GOOD;

  if (false == m_isInitialized) {
    LOG(LOG_FATAL, "Consumer Not Initialized");
    retCode = HARE_ERROR_E::NOT_INITIALIZED;
  }

  if (m_threadRunning) {
    LOG(LOG_ERROR, "Thread Already Running, cannot subscribe TODO");
    retCode = HARE_ERROR_E::THREAD_ALREADY_RUNNING;
  }

  if (noError(retCode)) {
    auto channel = m_channelHandler.addChannelProcessor(
        std::make_pair(exchange, binding_key), f);

    if (channel == -1) {
      char log[80];
      sprintf(log, "Unable to subscribe to %s : %s", exchange.c_str(),
              binding_key.c_str());
      LOG(LOG_ERROR, log);
      retCode = HARE_ERROR_E::UNABLE_TO_SUBSCRIBE;
    } else {
    }
    if (noError(retCode)) {
      m_channelHandler.setQueueProperties(channel, queueProps);
    }
  }
  return retCode;
}

HARE_ERROR_E Consumer::start() {
  std::lock_guard<std::mutex> lock(m_consumerMutex);
  auto retCode = HARE_ERROR_E::ALL_GOOD;

  LOG(LOG_DETAILED, "Consumer Thread Startup");

  if (false == m_isInitialized) {
    LOG(LOG_ERROR, "Consumer not Initialized");
    retCode = HARE_ERROR_E::NOT_INITIALIZED;
  }

  if (noError(retCode) && m_threadRunning) {
    LOG(LOG_WARN, "Thread already running");
    retCode = HARE_ERROR_E::THREAD_ALREADY_RUNNING;
  }

  if (noError(retCode)) {
    m_exitThreadSignal = new std::promise<void>();

    m_futureObj = m_exitThreadSignal->get_future();

    m_threadRunning = true;

    // Start up the consumer thread
    m_consumerThread = std::thread(&Consumer::thread, this);

    LOG(LOG_INFO, "Consumer Thread Started");
  }

  return retCode;
}

HARE_ERROR_E Consumer::stop() {
  auto retCode = HARE_ERROR_E::ALL_GOOD;

  if (false == m_isInitialized) {
    LOG(LOG_ERROR, "Consumer not initialized");
    retCode = HARE_ERROR_E::NOT_INITIALIZED;
  }

  if (noError(retCode) && false == m_threadRunning) {
    retCode = HARE_ERROR_E::THREAD_NOT_RUNNING;
  } else if (noError(retCode)) {
    m_threadRunning = false;
    LOG(LOG_WARN, "Consumer thread stopping");
    m_exitThreadSignal->set_value();
    m_consumerThread.join();

    delete m_exitThreadSignal;

    if (m_unboundChannelThreadRunning) {
      stopUnboundChannelThread();
    }

    // Stop consuming on all channels
    for (int channel : m_channelHandler.getChannelList()) {
      {
        char log[80];
        sprintf(log, "Stopping Consuming on channel: %d", channel);
        LOG(LOG_WARN, log);
      }
      m_connection->CloseChannel(channel);
    }
    retCode = m_connection->CloseConnection();
  }

  return retCode;
}

void Consumer::stopUnboundChannelThread() {
  if (m_unboundChannelThreadRunning) {
    LOG(LOG_WARN, "Unbound Channel Thread Stopping");
    m_unboundChannelThreadRunning = false;

    m_unboundChannelThreadSig->set_value();
    m_unboundChannelThread.join();

    delete m_unboundChannelThreadSig;
  }
}

/**
 * Intialize function
 */
HARE_ERROR_E Consumer::Initialize(const std::string& server, int port) {
  auto retCode = HARE_ERROR_E::ALL_GOOD;
  m_connection = std::make_shared<connection::ConnectionBase>(server, port);

  if (noError(retCode)) {
    m_isInitialized = true;
    m_threadRunning = false;
    m_unboundChannelThreadRunning = false;
    LOG(LOG_INFO, "Consumer Initialized Successfully")
  }
  return retCode;
}

HARE_ERROR_E Consumer::Restart() {
  auto retCode = HARE_ERROR_E::ALL_GOOD;
  retCode = stop();
  if (noError(retCode)) {
    retCode = start();
  }
  return retCode;
}

}  // namespace HareCpp