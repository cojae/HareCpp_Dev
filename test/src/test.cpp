#include "MessageTest.hpp"
#include "ConsumerAPITest.hpp"
#include "ConsumerSubscriptionTest.hpp"
#include "ProducerAPITest.hpp"
#include "ProducerConsumerTester.hpp"
#include "DeclareExchangeTest.hpp"
#include "MultiSubscribeTest.hpp"
#include "RestartTest.hpp"

int main(int argc, char** argv) {
  HareCpp::SET_DEBUG_LEVEL(HareCpp::LOG_NONE);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}