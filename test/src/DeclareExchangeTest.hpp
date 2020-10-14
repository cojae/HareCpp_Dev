#include "ProducerConsumerTester.hpp"

inline int testSomeMessageReceives(const int totalMsg, ProducerConsumerTester& pubsubTest) {
    pubsubTest.Restart();
    auto start = std::chrono::system_clock::now();
    while(pubsubTest.GetDesiredMessageCount() < totalMsg) {

        if(pubsubTest.GetUndesiredMessageCount() > 0)  
            return 0; 

        // Send a message over with the producer
        auto newMessage = HareCpp::Message("hello world");
        pubsubTest.producer.Send("newExchange","test",newMessage) ;

        auto cur = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsedTime = cur - start;
        // Kill after 5 seconds, we should receive 5 messages by now
        if (elapsedTime.count() > 10) 
            return 0;
    }
    return 1;
}

TEST_F(ProducerConsumerTester, exchangeDeclareTest) {
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, consumer.Subscribe("newExchange", "test",
                                 std::bind(&ProducerConsumerTester::defaultCallback,
                                           this, std::placeholders::_1)));
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, producer.DeclareExchange("newExchange"));

  producer.Start();
  consumer.Start();

  ASSERT_EQ(1, testSomeMessageReceives(5,*this));

}