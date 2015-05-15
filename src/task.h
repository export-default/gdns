#ifndef GDNS_PROXY_H
#define GDNS_PROXY_H

#include "common.h"

void task_init(query_task_t *task, upstream_proxy_t *proxy, char *msg, ssize_t len);

void task_run(uv_loop_t *loop, query_task_t *task, task_cb cb);

void task_close(query_task_t *task, task_close_cb close_cb);

#endif //GDNS_PROXY_H
