/*
 * libharecpp - Wrapper Library around: rabbitmq-c - rabbitmq C library
 *
 * Copyright (c) 2020 Cody Williams
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
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
 * NOTE: Never used a Pre-compiled header before.  I feel like i'm dong it
 * wrong, GNU seems to imply a different build command. The includes are also
 * sporatic and some classes are defining them themselves.  So is this
 * necessary? For now, to ease my life.
 *
 * Maybe change this completely later TODO
 */
constexpr int CONNECTION_TIMEOUT_SECONDS = 1;
constexpr int CONNECTION_RETRY_TIMEOUT_MILLISECONDS = 1000;

namespace HareCpp {
typedef std::function<void(const class Message&)> TD_Callback;
}
#endif
