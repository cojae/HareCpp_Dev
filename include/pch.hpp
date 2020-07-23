#ifndef __PCH__
#define __PCH__
extern "C" {
#include <amqp.h>
#include <amqp_framing.h>
#include <amqp_tcp_socket.h>
}

#include <functional>
#include <memory>
#include <mutex>
#include <string>

#include "ErrorCodes.hpp"
#include "Logger.hpp"

constexpr int DEFAULT_CONNECTION_TIMEOUT = 1;

namespace HareCpp {
typedef std::function<void(const class Message&)> TD_Callback;
}
#endif
