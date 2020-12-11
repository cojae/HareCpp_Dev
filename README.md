# HareCpp_Dev

Work in Progress

Published here for peer review purposes.  There are many issues that need resolved before final publish

# Purpose
An ease of access pub/sub library to use rabbitmq.  I am trying to make a library in which the user does not need to know the underlying AMQP protocol and instead use a very high level adoption of the api.

# Producer Class
Creates a publisher that receives Send requests with an exchange, a routing key, and a HareCpp::Message.  This message goes on a queue to be sent out, while a thread is running through said queue and publishing messages accordingly.

# Consumer Class
Creates a subscriber that allows the user to set a callback to a particular exchange/routing key combination.  Once a message is received of this type, the callback is called.  The callback being a `void callback_name(const HareCpp::Message& message)` function.
