#include "Producer.hpp"
#include "Constants.hpp"

TEST(ProducerTest, initializeProducer) {
  HareCpp::Producer producer;
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, producer.Initialize(
    SERVER, PORT, USERNAME, PASSWORD
  ));
}

TEST(ProducerTest, isInitializedTrue) {
  HareCpp::Producer producer;
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, producer.Initialize(
    SERVER, PORT, USERNAME, PASSWORD
  ));
  ASSERT_TRUE(producer.IsInitialized());
}

TEST(ProducerTest, isInitializedFalse) {
  HareCpp::Producer producer;
  ASSERT_FALSE(producer.IsInitialized());
}

TEST(ProducerTest, isRunningBeforeInitFalse) {
  HareCpp::Producer producer;
  ASSERT_FALSE(producer.IsRunning());
}

TEST(ProducerTest, isRunningAfterInitFalse) {
  HareCpp::Producer producer;
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, producer.Initialize(
    SERVER, PORT, USERNAME, PASSWORD
  ));
  ASSERT_FALSE(producer.IsRunning());
}

TEST(ProducerTest, isRunningAfterStart) {
  HareCpp::Producer producer;
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, producer.Initialize(
    SERVER, PORT, USERNAME, PASSWORD
  ));
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, producer.Start());
  ASSERT_TRUE(producer.IsRunning());
}

TEST(ProducerTest, startBeforeInit) {
  HareCpp::Producer producer;
  ASSERT_EQ(HareCpp::HARE_ERROR_E::NOT_INITIALIZED, producer.Start());
}

TEST(ProducerTest, startAfterInit) {
  HareCpp::Producer producer;
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, producer.Initialize(
    SERVER, PORT, USERNAME, PASSWORD
  ));
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, producer.Start());
}

TEST(ProducerTest, stopBeforeStart) {
  HareCpp::Producer producer;
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, producer.Initialize(
    SERVER, PORT, USERNAME, PASSWORD
  ));
  ASSERT_EQ(HareCpp::HARE_ERROR_E::THREAD_NOT_RUNNING, producer.Stop());
}

TEST(ProducerTest, stopAfterStart) {
  HareCpp::Producer producer;
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, producer.Initialize(
    SERVER, PORT, USERNAME, PASSWORD
  ));
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, producer.Start());
  ASSERT_TRUE(producer.IsRunning());
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, producer.Stop());
  ASSERT_FALSE(producer.IsRunning());
}

TEST(ProducerTest, restartBeforeStart) {
  HareCpp::Producer producer;
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, producer.Initialize(
    SERVER, PORT, USERNAME, PASSWORD
  ));
  ASSERT_EQ(HareCpp::HARE_ERROR_E::THREAD_NOT_RUNNING, producer.Restart());
}

TEST(ProducerTest, restartBeforeInit) {
  HareCpp::Producer producer;
  ASSERT_EQ(HareCpp::HARE_ERROR_E::NOT_INITIALIZED, producer.Restart());
}

TEST(ProducerTest, restartAfterStart) {
  HareCpp::Producer producer;
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, producer.Initialize(
    SERVER, PORT, USERNAME, PASSWORD
  ));
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, producer.Start());
  ASSERT_TRUE(producer.IsRunning());
  ASSERT_EQ(HareCpp::HARE_ERROR_E::ALL_GOOD, producer.Restart());
}

