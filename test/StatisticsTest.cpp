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

std::string generateJunk(int size) {                                                
  //int size = 13000000; // 13Mb

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
  double microsecondTotal; // Total time receiving messages

  std::mutex mutexy;

 public:
  testReceive()
      : messageCount(0),  microsecondTotal(0.0) {};
  void receiverCallback(const HareCpp::Message& message) {
    uint64_t microseconds_since_epoch = 
        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    mutexy.lock();
    microsecondTotal += (microseconds_since_epoch - message.getTimestamp());
    messageCount++;
    mutexy.unlock();
  };
  void restart() {
    mutexy.lock();
    messageCount = 0;
    microsecondTotal = 0;
    mutexy.unlock();
  };
  int getDropped() {
    std::lock_guard<std::mutex> lock(mutexy);
    return 1000 - messageCount;
  }
  double getAverageTime() {
    std::lock_guard<std::mutex> lock(mutexy);
    return microsecondTotal / messageCount;
  }
};

int main() {

  printf("Starting Statistical Analysis\n==================================\n");

  HareCpp::SET_DEBUG_LEVEL(HareCpp::LOG_FATAL);

  HareCpp::Producer producer;
  HareCpp::Consumer consumer;
  testReceive test;
  auto retCode = producer.initialize("rabbit-serv", 5672);
  retCode = consumer.Initialize("rabbit-serv", 5672);
  if (HareCpp::noError(retCode)) {
    retCode = producer.start();
    if (false == HareCpp::noError(retCode)) {
      return 0;
    }
    retCode = consumer.subscribe("amq.direct", "test",
                                 std::bind(&testReceive::receiverCallback,
                                           &test, std::placeholders::_1));
    if (false == HareCpp::noError(retCode)) {
      HareCpp::LOG(HareCpp::LOG_FATAL, "Failed");
      return 0;
    }

    retCode = consumer.start();
    if (false == HareCpp::noError(retCode)) {
      HareCpp::LOG(HareCpp::LOG_FATAL, "Failed");
      return 0;
    }
    printf("Running Tests:\n");
    for(int Hz = 8; Hz <= 128; Hz*=2) {
      int iter = 8;
      test.restart();

      for ( int i = 1 ; i < 24; i++ ) {
      //for ( int i = 1 ; i < 3; i++ ) {
        // Main test
        auto newMessage = HareCpp::Message(generateJunk(iter));
        if (false == HareCpp::noError(retCode)) {
          HareCpp::LOG(HareCpp::LOG_FATAL, "Failed");
          return 0;
        }
        int count = 1;
        printf("-----------------------------------------------\n");
        //printf("Test %d: Number of bytes per message (at 16 Hz): %d\n", i, i*1000000);
        printf("Test %d: Number of bytes per message (at %d Hz): %d bytes\n", i, Hz, iter);
        while (count <= 1000) {
          retCode = producer.send("amq.direct", "test", newMessage);
          std::this_thread::sleep_for(std::chrono::milliseconds(1000/Hz));
          //std::this_thread::sleep_for(std::chrono::milliseconds(62));
          count++;
        }
        iter *= 2;
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        int messageDrop = test.getDropped();
        double averageTime = test.getAverageTime();
        printf("Avg Microseconds till receive: %lf\n", averageTime);
        printf("Avg Seconds till receive: %lf\n", averageTime * .000001 );
        printf("Unreceived Messages: %d\n", messageDrop );
        printf("-----------------------------------------------\n");
        consumer.Restart();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        test.restart();
        producer.Restart();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      }
    }
    retCode = producer.stop();
    if (false == HareCpp::noError(retCode)) {
      HareCpp::LOG(HareCpp::LOG_FATAL, "Failed");
      return 0;
    }
    retCode = consumer.stop();
  }
  return 0;
};