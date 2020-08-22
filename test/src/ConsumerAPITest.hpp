#include "ConsumerTester.hpp"
#include "Constants.hpp"

TEST_F(ConsumerTester, initializeConsumer) {
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, consumer.Initialize(
    SERVER, PORT, USERNAME, PASSWORD
  ));
}

TEST_F(ConsumerTester, isInitializedTrue) {
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, consumer.Initialize(
    SERVER, PORT, USERNAME, PASSWORD
  ));
  ASSERT_TRUE(consumer.IsInitialized());
}

TEST_F(ConsumerTester, isInitializedFalse) {
  ASSERT_FALSE(consumer.IsInitialized());
}

TEST_F(ConsumerTester, isRunningBeforeInitFalse) {
  ASSERT_FALSE(consumer.IsRunning());
}

TEST_F(ConsumerTester, isRunningAfterInitFalse) {
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, consumer.Initialize(
    SERVER, PORT, USERNAME, PASSWORD
  ));
  ASSERT_FALSE(consumer.IsRunning());
}

TEST_F(ConsumerTester, isRunningAfterStart) {
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, consumer.Initialize(
    SERVER, PORT, USERNAME, PASSWORD
  ));
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, consumer.Start());
  ASSERT_TRUE(consumer.IsRunning());
}

TEST_F(ConsumerTester, startBeforeInit) {
  ASSERT_EQ(HareCpp::HARE_ERROR_E::NOT_INITIALIZED, consumer.Start());
}

TEST_F(ConsumerTester, startAfterInit) {
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, consumer.Initialize(
    SERVER, PORT, USERNAME, PASSWORD
  ));
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, consumer.Start());
}

TEST_F(ConsumerTester, stopBeforeStart) {
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, consumer.Initialize(
    SERVER, PORT, USERNAME, PASSWORD
  ));
  ASSERT_EQ(HareCpp::HARE_ERROR_E::THREAD_NOT_RUNNING, consumer.Stop());
}

TEST_F(ConsumerTester, stopAfterStart) {
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, consumer.Initialize(
    SERVER, PORT, USERNAME, PASSWORD
  ));
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, consumer.Start());
  ASSERT_TRUE(consumer.IsRunning());
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, consumer.Stop());
  ASSERT_FALSE(consumer.IsRunning());
}

TEST_F(ConsumerTester, restartBeforeStart) {
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, consumer.Initialize(
    SERVER, PORT, USERNAME, PASSWORD
  ));
  ASSERT_EQ(HareCpp::HARE_ERROR_E::THREAD_NOT_RUNNING, consumer.Restart());
}

TEST_F(ConsumerTester, restartBeforeInit) {
  ASSERT_EQ(HareCpp::HARE_ERROR_E::NOT_INITIALIZED, consumer.Restart());
}

TEST_F(ConsumerTester, restartAfterStart) {
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, consumer.Initialize(
    SERVER, PORT, USERNAME, PASSWORD
  ));
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, consumer.Start());
  ASSERT_TRUE(consumer.IsRunning());
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, consumer.Restart());
}