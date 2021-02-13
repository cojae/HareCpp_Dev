# HareCpp #

Development Repo, used for peer review and development purposes.  This will not be the official repository, but it will be created shortly

## Description ##
A C++ wrapper around rabbitmq-c library.  This wrapper's purpose is to abstract away the need for core understanding of the AMQP protocol and the RabbitMQ's implementation of it.  To give an easy usage of AMQP messaging, to get C++ applications up and using AMQP as quickly as possible.  

## Installation ##

- ### Requirements ###

  - Linux OS (Never tested nor developed for Windows/MacOS)
  - A C++ Compliler with c++11 capability (G++ 7.5+ and clang++ 6.0+ were used during testing)
  - librabbitmq-c (version 0.10.0 was used during development, but other more recent versions should still work)
  - _Optionally_ OpenSSL v0.9.8+ for connecting over SSL/TLS (though currently not implemented)
  - GNU Make for compilation, currently doesn't include a CMakeLists for Cmake.

- ### Instructions ###

  Using CMake:
  ```
  mkdir bld
  cd bld
  cmake ..
  make install
  ```
  Using GNU Make:
  ```
  cd src
  make install
  ```

  Both will install headers in `/usr/local/include/harecpp` and the compiled object in `/usr/local/lib/harecpp`

- ### Testing ###
  Currently no option to install test applications using cmake.  However, you can install it manually by going into the test directory and running `make test`.  It requires googletest to be installed and potentially correctly linking, also requires rabbitmq broker be running.

  When compiling, make sure you change `src/Constants.hpp` file to use your broker location (defaults to rabbit-serv but can be changed to localhost), and port used.  No way (currently) to run with command line arguments to tell it where rabbitmq is running (TODO)

## Usage ##

There are 3 main classes to use: `HareCpp::Producer`, `HareCpp::Consumer`, and `HareCpp::Message`.  
  
  - ### Producer ###
      Establishes a connection to rabbitmq and creates a queue accessable by the `Send()` api call.  This runs a thread that will pull from the queue and use rabbitmq-c api to send messages to the broker
  - ### Consumer ###
      Establishes a connection to rabbitmq and creates a consumer thread upon starting.  Prior to starting, its recommended to `Subscribe` to all exchanges/routing keys needed for messages.  It also requires a callback method be created and used in subscription: `void callback_name(const HareCpp::Message& message)`.  This function will be called upon receipt of a message, by the main Consumer thread.
  - ### Message ###
      Custom class to wrap around all necessary amqp message structures (used by rabbitmq-c), and give easy api calls to the internal data.  This class is used to check all necessary amqp message information.

## Release Info ##

Currently no release has been made, waiting on peer reviews and more testing/rewriting to support more use cases.