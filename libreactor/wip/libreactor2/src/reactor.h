#ifndef REACTOR_H_INCLUDED
#define REACTOR_H_INCLUDED

#define REACTOR_VERSION       "1.0.0"
#define REACTOR_VERSION_MAJOR 1
#define REACTOR_VERSION_MINOR 0
#define REACTOR_VERSION_PATCH 0

#define reactor_likely(x)     __builtin_expect(!!(x),1)
#define reactor_unlikely(x)   __builtin_expect(!!(x),0)

#ifdef __cplusplus
extern "C" {
#endif

#include "reactor/reactor_user.h"
#include "reactor/reactor_desc.h"
#include "reactor/reactor_core.h"
#include "reactor/reactor_async.h"
#include "reactor/reactor_timer.h"
#include "reactor/reactor_stream.h"
#include "reactor/reactor_resolver.h"
#include "reactor/reactor_tcp.h"
#include "reactor/reactor_http_parser.h"
#include "reactor/reactor_http.h"
#include "reactor/reactor_http_client.h"
#include "reactor/reactor_http_server.h"

#ifdef __cplusplus
}
#endif

#endif /* REACTOR_H_INCLUDED */
