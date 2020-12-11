#include "gtest/gtest.h"
#include "Message.hpp"
#include <string.h>

TEST(MessageTest, createBlankMessage) {
  HareCpp::Message message;
  ASSERT_EQ(0, message.Length());
}

/*
TEST(MessageTest, createFromEnvelope) {
  amqp_envelope_t envelope;
  ASSERT_TRUE(0);
}
*/

TEST(MessageTest, helloWorldMessageAsString) {
  std::string helloWorld("Hello World");
  HareCpp::Message message(helloWorld);
  ASSERT_EQ(helloWorld.size(), message.Length());
}

TEST(MessageTest, helloWorldMessageNoCopy) {
  HareCpp::Message message("Hello World");
  ASSERT_EQ(std::string("Hello World").size(), message.Length());
}

TEST(MessageTest, payloadAsChar) {
  HareCpp::Message message("Hello World");
  ASSERT_EQ(0,(strcmp("Hello World", message.Payload())));
}

TEST(MessageTest, payloadAsCharInvalid) {
  HareCpp::Message message;
  ASSERT_EQ(nullptr,message.Payload());
}

TEST(MessageTest, payloadAsString) {
  HareCpp::Message message("Hello World");
  ASSERT_EQ("Hello World", message.String());
}

TEST(MessageTest, payloadAsStringInvalid) {
  HareCpp::Message message;
  ASSERT_EQ("", message.String());
}

TEST(MessageTest, setPayloadChar) {
  HareCpp::Message message;
  const char* payload = "hello world";
  message.SetPayload(payload);
  ASSERT_EQ(0, strcmp("hello world", message.Payload()));
}

TEST(MessageTest, setPayloadVoid) {
  HareCpp::Message message;
  const char* payload = "hello world";
  message.SetPayload((void*)payload, strlen(payload)+1);
  ASSERT_EQ(0, strcmp("hello world", message.Payload()));
}


TEST(MessageTest, setReplyToFalse) {
  HareCpp::Message message;
  ASSERT_FALSE(message.ReplyToIsSet());
}

TEST(MessageTest, setReplyTo) {
  HareCpp::Message message;
  message.SetReplyTo("blah");
  ASSERT_TRUE(message.ReplyToIsSet());
}

TEST(MessageTest, verifyReplyTo) {
  HareCpp::Message message;
  message.SetReplyTo("blah");
  ASSERT_EQ(std::string("blah"),message.ReplyTo());
}

TEST(MessageTest, noCorrelationId) {
  HareCpp::Message message;
  ASSERT_FALSE(message.HasCorrelationId());
}

TEST(MessageTest, setCorrelationId) {
  HareCpp::Message message;
  message.SetCorrelationId("blah");
  ASSERT_TRUE(message.HasCorrelationId());
}

TEST(MessageTest, setCorrelationIdChar) {
  HareCpp::Message message;
  const char* tmp = std::string("blah").c_str(); //Because lazy
  message.SetCorrelationId(tmp);
  ASSERT_TRUE(message.HasCorrelationId());
}

/*
GetCorrelationId
*/

TEST(MessageTest, timeStampNotSet) {
  HareCpp::Message message;
  ASSERT_FALSE(message.TimestampIsSet());
}

TEST(MessageTest, setTimeStamp) {
  HareCpp::Message message;
  message.SetTimestamp(35345);
  ASSERT_TRUE(message.TimestampIsSet());
}

TEST(MessageTest, getTimeStamp) {
  uint64_t bsTime{324523};
  HareCpp::Message message;
  message.SetTimestamp(bsTime);
  ASSERT_EQ(bsTime, message.Timestamp());
}

TEST(MessageTest, copyConstructor) {
  HareCpp::Message message1;
  HareCpp::Message message2{message1};
  ASSERT_EQ(message1.Length(), message2.Length());
}

TEST(MessageTest, copyMessage) {
  HareCpp::Message message1;
  HareCpp::Message message2 = message1;
  ASSERT_EQ(message1.Length(), message2.Length());
}

TEST(MessageTest, copyMessageModifyCopy) {
  HareCpp::Message message1;
  message1.SetPayload("blahblah");
  HareCpp::Message message2 = message1;
  message2.SetPayload("blah");
  ASSERT_FALSE(message1.Length() == message2.Length());
}