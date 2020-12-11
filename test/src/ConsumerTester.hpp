#include "Consumer.hpp"
#include "gtest/gtest.h"
#include <string.h>

#ifndef CONSUMERTESTER
#define CONSUMERTESTER
class ConsumerTester : public ::testing::Test {
  private:
    int m_desiredMessageCount;
    int m_undesiredMessageCount;
    std::string m_desiredMessage;
    std::mutex m_theMutex;
  public:
    HareCpp::Consumer consumer;
    void SetUp() { }

    ConsumerTester() : m_desiredMessageCount(0), m_undesiredMessageCount(0), m_desiredMessage("hello world"){};

    void defaultCallback(const HareCpp::Message& message) {
      m_theMutex.lock();
      if( (strcmp(message.Payload() , m_desiredMessage.c_str())) ||
        (std::string(message.Payload()) == m_desiredMessage )) m_desiredMessageCount++;
      else {
        m_undesiredMessageCount++;
        printf("Failure: '%s' : '%s'\n",message.Payload(), m_desiredMessage.c_str());
      }
      m_theMutex.unlock();
    };

    void SetDesiredMessage(const std::string& message) {
      m_desiredMessage = message;
    };
    int GetDesiredMessageCount() { const std::lock_guard<std::mutex> lock(m_theMutex); return m_desiredMessageCount; };
    int GetUndesiredMessageCount() { const std::lock_guard<std::mutex> lock(m_theMutex); return m_undesiredMessageCount; };
    void Restart() { 
      const std::lock_guard<std::mutex> lock(m_theMutex);
      m_desiredMessageCount = 0;
      m_undesiredMessageCount = 0;
    };
};
#endif