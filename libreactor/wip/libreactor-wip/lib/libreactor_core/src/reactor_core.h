#ifndef REACTORE_CORE_H_INCLUDED
#define REACTORE_CORE_H_INCLUDED

#define REACTORE_CORE_VERSION "0.2.0"
#define REACTORE_CORE_VERSION_MAJOR 0
#define REACTORE_CORE_VERSION_MINOR 2
#define REACTORE_CORE_VERSION_PATCH 0

#ifdef __cplusplus
extern "C" {
#endif

#include "reactor_core/reactor_user.h"
#include "reactor_core/reactor_desc.h"
#include "reactor_core/reactor_core.h"
#include "reactor_core/reactor_timer.h"
#include "reactor_core/reactor_stream.h"
#include "reactor_core/reactor_signal.h"
#include "reactor_core/reactor_signal_dispatcher.h"
#include "reactor_core/reactor_resolver.h"
#include "reactor_core/reactor_tcp_client.h"
#include "reactor_core/reactor_tcp_server.h"
#include "reactor_core/reactor_http_server.h"
#include "reactor_core/reactor_rest.h"

#ifdef __cplusplus
}
#endif

#endif /* REACTORE_CORE_H_INCLUDED */
