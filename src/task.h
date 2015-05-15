#ifndef GDNS_PROXY_H
#define GDNS_PROXY_H

#include "common.h"

void task_init(query_task_t *task, session_ctx_t *ctx, upstream_proxy_t *proxy, char *msg, ssize_t len, task_cb cb);

void task_run(uv_loop_t *loop, query_task_t *task);

void task_close(query_task_t *task);

#endif //GDNS_PROXY_H
