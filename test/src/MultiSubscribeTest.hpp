#include "ProducerConsumerTester.hpp"

inline int testMultiSubscribe(const int totalMsg, ProducerConsumerTester& pubsubTest) {
    pubsubTest.Restart();
    auto start = std::chrono::system_clock::now();
    while(pubsubTest.GetDesiredMessageCount() < totalMsg) {

        if(pubsubTest.GetUndesiredMessageCount() > 0)  
            return -1; 

        // Send a message over with the producer
        auto newMessage = HareCpp::Message("hello world");
        pubsubTest.producer.Send("exchange1","test",newMessage) ;

        auto cur = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsedTime = cur - start;
        // Kill after 5 seconds, we should receive 5 messages by now
        if (elapsedTime.count() > 5) 
            return 0;

    }
    pubsubTest.Restart();

    start = std::chrono::system_clock::now();
    while(pubsubTest.GetDesiredMessageCount() < totalMsg) {

        if(pubsubTest.GetUndesiredMessageCount() > 0)  
            return -1; 

        // Send a message over with the producer
        auto newMessage = HareCpp::Message("hello world");
        pubsubTest.producer.Send("exchange2","test",newMessage) ;

        auto cur = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsedTime = cur - start;
        // Kill after 5 seconds, we should receive 5 messages by now
        if (elapsedTime.count() > 5) 
            return 0;

    }

    return 1;
}

TEST_F(ProducerConsumerTester, multiSubscribeTest) {
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, consumer.Subscribe("exchange1", "test",
                                 std::bind(&ProducerConsumerTester::defaultCallback,
                                           this, std::placeholders::_1)));
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, consumer.Subscribe("exchange2", "test",
                                 std::bind(&ProducerConsumerTester::defaultCallback,
                                           this, std::placeholders::_1)));
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, producer.DeclareExchange("exchange1"));
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, producer.DeclareExchange("exchange2"));

  producer.Start();
  consumer.Start();

  // Get everything set up, 2 exchanges need declared and set, while the consumer
  // is periodically restarting, might throw off the timing
  std::this_thread::sleep_for(std::chrono::milliseconds(3000));

  ASSERT_EQ(1, testMultiSubscribe(5,*this));

}