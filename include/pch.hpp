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


/**
 * NOTE: Never used a Pre-compiled header before.  I feel like i'm dong it wrong, GNU seems to imply a different build command.  
 * The includes are also sporatic and some classes are defining them themselves.  So is this necessary? For now, to ease my life.
 * 
 * Maybe change this completely later TODO
 */
constexpr int DEFAULT_CONNECTION_TIMEOUT = 1;

namespace HareCpp {
typedef std::function<void(const class Message&)> TD_Callback;
}
#endif
