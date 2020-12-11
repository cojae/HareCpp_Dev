#include "ConsumerTester.hpp"
#include "Producer.hpp"

inline int testSomeMessageReceives(const int totalMsg, ConsumerTester& consumerTest) {
    consumerTest.Restart();
    HareCpp::Producer producer;
    producer.Initialize(SERVER,PORT, USERNAME,PASSWORD) ;
    producer.Start();
    auto start = std::chrono::system_clock::now();
    while(consumerTest.GetDesiredMessageCount() < totalMsg) {

        if(consumerTest.GetUndesiredMessageCount() > 0)  
            return -1; 

        // Send a message over with the producer
        auto newMessage = HareCpp::Message("hello world");
        producer.Send("amq.direct","test",newMessage) ;

        auto cur = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsedTime = cur - start;
        // Kill after 5 seconds, we should receive 5 messages by now
        if (elapsedTime.count() > 5) 
            return 0;

    }
    return 1;
}

TEST_F(ConsumerTester, basicSubscribeTest) {
  consumer.Initialize(
    SERVER, PORT, USERNAME, PASSWORD );
  
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, consumer.Subscribe("amq.direct", "test",
                                 std::bind(&ConsumerTester::defaultCallback,
                                           this, std::placeholders::_1)));
  
  consumer.Start();
  ASSERT_EQ(1, testSomeMessageReceives(5, *this));
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, consumer.Stop());
  ASSERT_FALSE(consumer.IsRunning());
}

TEST_F(ConsumerTester, startAndStopSubscribeTest) {
  consumer.Initialize(
    SERVER, PORT, USERNAME, PASSWORD );
  
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, consumer.Subscribe("amq.direct", "test",
                                 std::bind(&ConsumerTester::defaultCallback,
                                           this, std::placeholders::_1)));
  consumer.Start();
  ASSERT_EQ(1, testSomeMessageReceives(5, *this));
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, consumer.Stop());
  ASSERT_FALSE(consumer.IsRunning());

  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, consumer.Start());
  ASSERT_EQ(1, testSomeMessageReceives(5, *this));

  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, consumer.Stop());
  ASSERT_FALSE(consumer.IsRunning());
}