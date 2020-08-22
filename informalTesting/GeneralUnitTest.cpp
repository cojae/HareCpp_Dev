#include <assert.h>
#include <unistd.h>
#include <chrono>
#include <thread>
#include "Consumer.hpp"
#include "Producer.hpp"

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

std::string generateJunk() {                                                
  int size = 13000000; // 13Mb

  char *random = new char[size];
  for ( int i = 0; i < size-1; i++ ) {
    random[i] = 'a';
  }
 random[size-1] = '\0';
 std::string retVal(random);
 free(random);
 return retVal;
}

class testReceive {
 private:
  int messageCount;
  int messageCountTwo;
  double microsecondTotal; // Total time receiving messages
  std::string desiredMessage;

  std::mutex mutexy;

 public:
  testReceive(std::string desiredMessage)
      : messageCount(0), messageCountTwo(0), microsecondTotal(0.0), desiredMessage(desiredMessage){};
  void receiverCallback(const HareCpp::Message& message) {
    mutexy.lock();
    assert(*message.payload() == desiredMessage);
    messageCount++;
    mutexy.unlock();
  };
  void receiverSecondCallback(const HareCpp::Message& message) {
    assert(*message.payload() == desiredMessage);
    mutexy.lock();
    messageCountTwo++;
    mutexy.unlock();
  };
  void receiverJunkCallback(const HareCpp::Message& message) {
    uint64_t microseconds_since_epoch = 
        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    if( generateJunk() == *message.payload()) {
      //printf("-------------------------------------------\n");
      //printf("Microseconds till receive: %lu\n", (microseconds_since_epoch - message.getTimestamp()) );
      //printf("Seconds till receive: %f\n", (microseconds_since_epoch - message.getTimestamp()) * .000001 );
      microsecondTotal += (microseconds_since_epoch - message.getTimestamp());
      //printf("%s\n",message.payload()->c_str()) ;
      messageCount++;
    }
  };
  void restart() {
    mutexy.lock();
    messageCount = 0;
    messageCountTwo = 0;
    mutexy.unlock();
  };
  bool isComplete() { return messageCount >= 3; };
  bool isLongComplete() { return messageCount >= 300; };
  bool advancedIsComplete() {
    return (messageCount >= 3 && messageCountTwo >= 3);
  };
  double getAverageTime() {
    return microsecondTotal / messageCount;
  }
};
int foreverTest() {
  HareCpp::Producer producer;
  HareCpp::Consumer consumer;
  testReceive test("test message");
  auto retCode = producer.Initialize("rabbit-serv", 5672);
  retCode = consumer.Initialize("rabbit-serv", 5672);
  if (HareCpp::noError(retCode)) {
    retCode = producer.Start();
    if (false == HareCpp::noError(retCode)) {
      return 0;
    }
    retCode = consumer.Subscribe("amq.direct", "test",
                                 std::bind(&testReceive::receiverCallback,
                                           &test, std::placeholders::_1));
    if (false == HareCpp::noError(retCode)) {
      return 0;
    }

    retCode = consumer.Start();
    if (false == HareCpp::noError(retCode)) {
      return 0;
    }

    auto newMessage = HareCpp::Message("test message");
    if (false == HareCpp::noError(retCode)) {
      return 0;
    }
    while (1) {
      retCode = producer.Send("amq.direct", "test", newMessage);
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    retCode = producer.Stop();
    if (false == HareCpp::noError(retCode)) {
      return 0;
    }
    retCode = consumer.Stop();
    return (HareCpp::noError(retCode));
  }
  return 0;
};

int basicSubscribeTest() {
  HareCpp::Producer producer;
  HareCpp::Consumer consumer;
  testReceive test("test message");
  auto retCode = producer.Initialize("rabbit-serv", 5672);
  retCode = consumer.Initialize("rabbit-serv", 5672);
  if (HareCpp::noError(retCode)) {
    retCode = producer.Start();
    if (false == HareCpp::noError(retCode)) {
      return 0;
    }
    retCode = consumer.Subscribe("amq.direct", "test",
                                 std::bind(&testReceive::receiverCallback,
                                           &test, std::placeholders::_1));
    if (false == HareCpp::noError(retCode)) {
      return 0;
    }

    retCode = consumer.Start();
    if (false == HareCpp::noError(retCode)) {
      return 0;
    }

    auto newMessage = HareCpp::Message("test message");
    if (false == HareCpp::noError(retCode)) {
      return 0;
    }
    auto start = std::chrono::system_clock::now();
    while (false == test.isComplete()) {
      retCode = producer.Send("amq.direct", "test", newMessage);
      auto cur = std::chrono::system_clock::now();
      std::chrono::duration<double> elapsedTime = cur - start;
      if (elapsedTime.count() > 5) {
        producer.Stop();
        consumer.Stop();
        return 0;
      }
    }
    retCode = producer.Stop();
    if (false == HareCpp::noError(retCode)) {
      return 0;
    }
    retCode = consumer.Stop();
    return (HareCpp::noError(retCode));
  }
  return 0;
};

int basicStartAndStop() {
  HareCpp::Producer producer;
  HareCpp::Consumer consumer;
  testReceive test("test message");
  auto retCode = producer.Initialize("rabbit-serv", 5672);
  retCode = consumer.Initialize("rabbit-serv", 5672);
  if (HareCpp::noError(retCode)) {
    retCode = producer.Start();
    if (false == HareCpp::noError(retCode)) {
      return 0;
    }
    retCode = consumer.Subscribe("amq.direct", "restartTest",
                                 std::bind(&testReceive::receiverCallback,
                                           &test, std::placeholders::_1));
    if (false == HareCpp::noError(retCode)) {
      return 0;
    }

    retCode = consumer.Start();
    if (false == HareCpp::noError(retCode)) {
      return 0;
    }

    auto newMessage = HareCpp::Message("test message");
    if (false == HareCpp::noError(retCode)) {
      return 0;
    }
    auto start = std::chrono::system_clock::now();
    while (false == test.isComplete()) {
      retCode = producer.Send("amq.direct", "restartTest", newMessage);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      auto cur = std::chrono::system_clock::now();
      std::chrono::duration<double> elapsedTime = cur - start;
      if (elapsedTime.count() > 5) {
        printf("time to die\n");
        producer.Stop();
        consumer.Stop();
        return 0;
      }
    }
    consumer.Stop();
    retCode = consumer.Subscribe("amq.direct", "secondTest",
                                 std::bind(&testReceive::receiverSecondCallback,
                                           &test, std::placeholders::_1));
    if (false == HareCpp::noError(retCode)) {
      return 0;
    }
    consumer.Start();
    test.restart();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    start = std::chrono::system_clock::now();
    while (false == test.advancedIsComplete()) {
      retCode = producer.Send("amq.direct", "restartTest", newMessage);
      retCode = producer.Send("amq.direct", "secondTest", newMessage);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      auto cur = std::chrono::system_clock::now();
      std::chrono::duration<double> elapsedTime = cur - start;
      if (elapsedTime.count() > 5) {
        producer.Stop();
        consumer.Stop();
        return 0;
      }
    }
    retCode = producer.Stop();
    if (false == HareCpp::noError(retCode)) {
      return 0;
    }
    retCode = consumer.Stop();
    return (HareCpp::noError(retCode));
  }
  return 0;
};

int basicRestart() {
  HareCpp::Producer producer;
  HareCpp::Consumer consumer;
  testReceive test("test message");
  auto retCode = producer.Initialize("rabbit-serv", 5672);
  retCode = consumer.Initialize("rabbit-serv", 5672);
  if (HareCpp::noError(retCode)) {
    retCode = producer.Start();
    if (false == HareCpp::noError(retCode)) {
      return 0;
    }
    retCode = consumer.Subscribe("amq.direct", "restartTest",
                                 std::bind(&testReceive::receiverCallback,
                                           &test, std::placeholders::_1));
    if (false == HareCpp::noError(retCode)) {
      return 0;
    }
    retCode = consumer.Subscribe("amq.direct", "secondTest",
                                 std::bind(&testReceive::receiverSecondCallback,
                                           &test, std::placeholders::_1));
    if (false == HareCpp::noError(retCode)) {
      return 0;
    }

    retCode = consumer.Start();
    if (false == HareCpp::noError(retCode)) {
      return 0;
    }

    auto newMessage = HareCpp::Message("test message");
    if (false == HareCpp::noError(retCode)) {
      return 0;
    }
    auto start = std::chrono::system_clock::now();
    while (false == test.isComplete()) {
      retCode = producer.Send("amq.direct", "restartTest", newMessage);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      auto cur = std::chrono::system_clock::now();
      std::chrono::duration<double> elapsedTime = cur - start;
      if (elapsedTime.count() > 5) {
        printf("time to die\n");
        producer.Stop();
        consumer.Stop();
        return 0;
      }
    }
    consumer.Restart();
    test.restart();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    start = std::chrono::system_clock::now();
    while (false == test.advancedIsComplete()) {
      retCode = producer.Send("amq.direct", "restartTest", newMessage);
      retCode = producer.Send("amq.direct", "secondTest", newMessage);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      auto cur = std::chrono::system_clock::now();
      std::chrono::duration<double> elapsedTime = cur - start;
      if (elapsedTime.count() > 5) {
        producer.Stop();
        consumer.Stop();
        return 0;
      }
    }
    retCode = producer.Stop();
    if (false == HareCpp::noError(retCode)) {
      return 0;
    }
    retCode = consumer.Stop();
    return (HareCpp::noError(retCode));
  }
  return 0;
};

int advancedSubscribeTest() {
  HareCpp::Producer producer;
  HareCpp::Consumer consumer;
  testReceive test("test message");
  auto retCode = producer.Initialize("rabbit-serv", 5672);
  retCode = consumer.Initialize("rabbit-serv", 5672);
  if (HareCpp::noError(retCode)) {
    retCode = producer.Start();
    if (false == HareCpp::noError(retCode)) {
      return 0;
    }
    retCode = consumer.Subscribe("amq.direct", "test",
                                 std::bind(&testReceive::receiverCallback,
                                           &test, std::placeholders::_1));
    if (false == HareCpp::noError(retCode)) {
      return 0;
    }
    retCode = consumer.Subscribe("amq.direct", "secondTest",
                                 std::bind(&testReceive::receiverSecondCallback,
                                           &test, std::placeholders::_1));
    if (false == HareCpp::noError(retCode)) {
      return 0;
    }

    retCode = consumer.Start();
    if (false == HareCpp::noError(retCode)) {
      return 0;
    }

    auto newMessage = HareCpp::Message("test message");
    if (false == HareCpp::noError(retCode)) {
      return 0;
    }
    auto start = std::chrono::system_clock::now();
    while (false == test.advancedIsComplete()) {
      retCode = producer.Send("amq.direct", "test", newMessage);
      retCode = producer.Send("amq.direct", "secondTest", newMessage);
      auto cur = std::chrono::system_clock::now();
      std::chrono::duration<double> elapsedTime = cur - start;
      if (elapsedTime.count() > 5) {
        producer.Stop();
        consumer.Stop();
        return 0;
      }
    }
    retCode = producer.Stop();
    if (false == HareCpp::noError(retCode)) {
      return 0;
    }
    retCode = consumer.Stop();
    return (HareCpp::noError(retCode));
  }
  return 0;
};

int basicExchangeDeclaration() {
  HareCpp::Producer producer;
  HareCpp::Consumer consumer;
  testReceive test("test message");
  auto retCode = producer.Initialize("rabbit-serv", 5672);
  retCode = consumer.Initialize("rabbit-serv", 5672);
  if (HareCpp::noError(retCode)) {
    retCode = producer.Start();
    if (false == HareCpp::noError(retCode)) {
      return 0;
    }
    retCode = consumer.Subscribe("blahblah", "test",
                                 std::bind(&testReceive::receiverCallback,
                                           &test, std::placeholders::_1));
    if (false == HareCpp::noError(retCode)) {
      return 0;
    }

    retCode = consumer.Start();
    if (false == HareCpp::noError(retCode)) {
      return 0;
    }

    producer.DeclareExchange("blahblah", "direct");

    auto newMessage = HareCpp::Message("test message");
    if (false == HareCpp::noError(retCode)) {
      return 0;
    }
    auto start = std::chrono::system_clock::now();
    while (false == test.isComplete()) {
      // So consumer thread can catch up
      retCode = producer.Send("blahblah", "test", newMessage);
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      auto cur = std::chrono::system_clock::now();
      std::chrono::duration<double> elapsedTime = cur - start;
      if (elapsedTime.count() > 10) {
        producer.Stop();
        consumer.Stop();
        return 0;
      }
    }
    retCode = producer.Stop();
    if (false == HareCpp::noError(retCode)) {
      return 0;
    }
    retCode = consumer.Stop();
    return (HareCpp::noError(retCode));
  }
  return 0;
}

int basicConsumerSetupTest() {
  HareCpp::Consumer test;
  auto retCode = test.Initialize("rabbit-serv", 5672);
  if (HareCpp::noError(retCode)) {
    retCode = test.Start();
    if (false == HareCpp::noError(retCode)) {
      printf(ANSI_COLOR_RED "Failed in starting thread\n" ANSI_COLOR_RESET);
      return 0;
    }
    retCode = test.Stop();
    if (false == HareCpp::noError(retCode)) {
      printf(ANSI_COLOR_RED "Failed in stopping thread\n" ANSI_COLOR_RESET);
      return 0;
    }
    return 1;
  }
  return 0;
}

int doubleThreadStartConsumer() {
  HareCpp::Consumer test;
  auto retCode = test.Initialize("rabbit-serv", 5672);
  if (HareCpp::noError(retCode)) {
    retCode = test.Start();
    if (false == HareCpp::noError(retCode)) {
      printf(ANSI_COLOR_RED "Failed in starting thread\n" ANSI_COLOR_RESET);
      return 0;
    }
    retCode = test.Start();  // Start a second time
    test.Stop();
    return (retCode == HareCpp::HARE_ERROR_E::THREAD_ALREADY_RUNNING);
  }
  return 0;
}

int basicProducerSetupTest() {
  HareCpp::Producer test;
  auto retCode = test.Initialize("rabbit-serv", 5672);
  if (HareCpp::noError(retCode)) {
    retCode = test.Start();
  }
  retCode = test.Stop();
  return (HareCpp::noError(retCode));
}

int basicProducerSendTest() {
  HareCpp::Producer test;
  auto retCode = test.Initialize("rabbit-serv", 5672);
  if (HareCpp::noError(retCode)) {
    retCode = test.Start();
  }
  if (HareCpp::noError(retCode)) {
    auto newMessage = HareCpp::Message("this is a test");
    retCode = test.Send("amq.direct", "testKey", newMessage);
  }
  if (HareCpp::noError(retCode)) {
    auto newMessage = HareCpp::Message("this is another test");
    retCode = test.Send("testKey", newMessage);
  }
  return (HareCpp::noError(retCode));
}

int largeDataSendTest() {
  HareCpp::Producer producer;
  HareCpp::Consumer consumer;
  testReceive test("test message");
  auto retCode = producer.Initialize("rabbit-serv", 5672);
  retCode = consumer.Initialize("rabbit-serv", 5672);
  if (HareCpp::noError(retCode)) {
    retCode = producer.Start();
    if (false == HareCpp::noError(retCode)) {
      return 0;
    }
    retCode = consumer.Subscribe("amq.direct", "test",
                                 std::bind(&testReceive::receiverJunkCallback,
                                           &test, std::placeholders::_1));
    if (false == HareCpp::noError(retCode)) {
      return 0;
    }

    retCode = consumer.Start();
    if (false == HareCpp::noError(retCode)) {
      return 0;
    }
    
    for( int i = 0; i < 3; i++) {
      //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      //printf("%d\n", i);
      auto newMessage = HareCpp::Message(generateJunk());
      newMessage.setTimestamp(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
      retCode = producer.Send("amq.direct", "test", newMessage);
    }
    if (false == HareCpp::noError(retCode)) {
      return 0;
    }
    auto start = std::chrono::system_clock::now();
    while (false == test.isComplete()) {
      auto cur = std::chrono::system_clock::now();
      std::chrono::duration<double> elapsedTime = cur - start;
      if (elapsedTime.count() > 1000) {
        producer.Stop();
        consumer.Stop();
        return 0;
      }
    }
    //printf("Average seconds : %lf\n", test.getAverageTime() * .000001);
    retCode = producer.Stop();
    if (false == HareCpp::noError(retCode)) {
      return 0;
    }
    retCode = consumer.Stop();
    return (HareCpp::noError(retCode));
  }
  return 0;
};

int main() {
  HareCpp::SET_DEBUG_LEVEL(HareCpp::LOG_FATAL);

  //while ( 1) {



  // TODO user input for server/port
  printf(ANSI_COLOR_GREEN
         "Running basic consumer setup test\n" ANSI_COLOR_RESET);
  if (0 == basicConsumerSetupTest()) {
    printf(ANSI_COLOR_RED
           "Failed basic consumer setup test\n" ANSI_COLOR_RESET);
  } else {
    printf(ANSI_COLOR_CYAN "SUCCESS!!\n" ANSI_COLOR_RESET);
  }

  // For dramatic effect
  //std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  printf(ANSI_COLOR_GREEN
         "Running basic consumer double thread start test\n" ANSI_COLOR_RESET);
  if (0 == doubleThreadStartConsumer()) {
    printf(ANSI_COLOR_RED
           "Failed basic consumer setup test\n" ANSI_COLOR_RESET);
  } else {
    printf(ANSI_COLOR_CYAN "SUCCESS!!\n" ANSI_COLOR_RESET);
  }

  // For dramatic effect
  //std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  printf(ANSI_COLOR_GREEN
         "Running basic producer setup test\n" ANSI_COLOR_RESET);
  if (0 == basicProducerSetupTest()) {
    printf(ANSI_COLOR_RED
           "Failed basic producer setup test\n" ANSI_COLOR_RESET);
  } else {
    printf(ANSI_COLOR_CYAN "SUCCESS!!\n" ANSI_COLOR_RESET);
  }

  // For dramatic effect
  //std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  printf(ANSI_COLOR_GREEN
         "Running basic producer send test\n" ANSI_COLOR_RESET);
  if (0 == basicProducerSendTest()) {
    printf(ANSI_COLOR_RED "Failed basic producer send test\n" ANSI_COLOR_RESET);
  } else {
    printf(ANSI_COLOR_CYAN "SUCCESS!!\n" ANSI_COLOR_RESET);
  }

  // For dramatic effect
  //std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  printf(ANSI_COLOR_GREEN "Running basic subscribe test\n" ANSI_COLOR_RESET);
  if (0 == basicSubscribeTest()) {
    printf(ANSI_COLOR_RED "Failed basic subscribe test\n" ANSI_COLOR_RESET);
  } else {
    printf(ANSI_COLOR_CYAN "SUCCESS!!\n" ANSI_COLOR_RESET);
  }

  // For dramatic effect
  //std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  printf(ANSI_COLOR_GREEN "Running advanced subscribe test\n" ANSI_COLOR_RESET);
  if (0 == advancedSubscribeTest()) {
    printf(ANSI_COLOR_RED "Failed advanced subscribe test\n" ANSI_COLOR_RESET);
  } else {
    printf(ANSI_COLOR_CYAN "SUCCESS!!\n" ANSI_COLOR_RESET);
  }

  // For dramatic effect
  //std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  printf(ANSI_COLOR_GREEN
         "Running basic exchange declaration test\n" ANSI_COLOR_RESET);
  if (0 == basicExchangeDeclaration()) {
    printf(ANSI_COLOR_RED
           "Failed basic exchange declaration test\n" ANSI_COLOR_RESET);
  } else {
    printf(ANSI_COLOR_CYAN "SUCCESS!!\n" ANSI_COLOR_RESET);
  }

  // For dramatic effect
  //std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  printf(ANSI_COLOR_GREEN
         "Running basic restart test\n" ANSI_COLOR_RESET);
  if (0 == basicRestart()) {
    printf(ANSI_COLOR_RED
           "Failed basic restart test\n" ANSI_COLOR_RESET);
  } else {
    printf(ANSI_COLOR_CYAN "SUCCESS!!\n" ANSI_COLOR_RESET);
  }

  // For dramatic effect
  //std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  printf(ANSI_COLOR_GREEN
         "Running basic stop and start test\n" ANSI_COLOR_RESET);
  if (0 == basicStartAndStop()) {
    printf(ANSI_COLOR_RED
           "Failed basic stop and start test\n" ANSI_COLOR_RESET);
  } else {
    printf(ANSI_COLOR_CYAN "SUCCESS!!\n" ANSI_COLOR_RESET);
  }

  // For dramatic effect
  //std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  printf(ANSI_COLOR_GREEN
         "Running large data (13Mb) send test\n" ANSI_COLOR_RESET);
  if (0 == largeDataSendTest()) {
    printf(ANSI_COLOR_RED
           "Failed large data send test\n" ANSI_COLOR_RESET);
  } else {
    printf(ANSI_COLOR_CYAN "SUCCESS!!\n" ANSI_COLOR_RESET);
  }

/*
  // For dramatic effect
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  printf(ANSI_COLOR_GREEN
         "Running forever test\n" ANSI_COLOR_RESET);
  if (0 == foreverTest()) {
    printf(ANSI_COLOR_RED
           "Failed forever test\n" ANSI_COLOR_RESET);
  } else {
    printf(ANSI_COLOR_CYAN "SUCCESS!!\n" ANSI_COLOR_RESET);
  }
  */
//  }

  return 0;
}