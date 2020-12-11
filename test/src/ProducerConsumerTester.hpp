#include "Producer.hpp"
#include "Consumer.hpp"
#include "Constants.hpp"

#include "gtest/gtest.h"
#include "string.h"

#ifndef PRODUCERCONSUMERTESTER
#define PRODUCERCONSUMERTESTER

class ProducerConsumerTester : public ::testing::Test {
  private:
    int m_desiredMessageCount;
    int m_undesiredMessageCount;
    std::string m_desiredMessage;
    std::mutex m_theMutex;
  public:
    HareCpp::Consumer consumer;
    HareCpp::Producer producer;
    void SetUp() { consumer.Initialize(SERVER); producer.Initialize(SERVER);}

    ProducerConsumerTester() : m_desiredMessageCount(0), m_undesiredMessageCount(0), m_desiredMessage("hello world"){};

    void defaultCallback(const HareCpp::Message& message) {
      m_theMutex.lock();
      if(strcmp(message.Payload() , m_desiredMessage.c_str()) ||
        (std::string(message.Payload()) == m_desiredMessage )) m_desiredMessageCount++;
      else m_undesiredMessageCount++;
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