cmake_minimum_required(VERSION 3.10)

project(harecpp)

set(GLOBAL_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set (CMAKE_CXX_COMPILER "/usr/bin/clang++")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set (CMAKE_CXX_COMPILER "/usr/bin/g++")
else()
  message(FATAL_ERROR "No compatable c++ compiler")
endif()

set (CMAKE_CXX_FLAGS "-std=c++11 -Wall -O2")
set (CMAKE_CXX_FLAGS_DEBUG_INIT "-g -Wall -O2 -Wextra")

#find rabbitmq-c
find_path(RABBITMQ_INCLUDE_DIR NAMES amqp.h)
if(NOT RABBITMQ_INCLUDE_DIR)
message(FATAL_ERROR "Rabbitmq include files not found")
endif()

find_library(RABBITMQ_LIBRARY
  NAMES librabbitmq rabbitmq 
  PATHS /usr/local/lib
        /usr/local/lib64
        /usr/lib
        /usr/lib64
        /usr/local/lib/x86_64-linux-gnu
)
if(NOT RABBITMQ_LIBRARY)
  message(FATAL_ERROR "Rabbitmq library not found")
endif()


set(PROJECT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)
set(PROJECT_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)


include_directories(${PROJECT_INCLUDE_DIR})
include_directories(SYSTEM /usr/local/include)

file(GLOB harecpp_src
  "${PROJECT_SOURCE_DIR}/*.cpp"
)

file(GLOB harecpp_inc
  "${PROJECT_INCLUDE_DIR}/*.hpp"
)

set(CMAKE_INSTALL_LIBDIR "/usr/local/lib/harecpp")
set(CMAKE_INSTALL_INCLUDEDIR "/usr/local/include/harecpp")


add_library(
  harecpp
  SHARED
  ${harecpp_src}
)
install(TARGETS harecpp
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES ${harecpp_inc} 
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

target_link_libraries(harecpp -lrabbitmq Threads::Threads)