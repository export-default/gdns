#ifndef GDNS_PROXY_H
#define GDNS_PROXY_H

#include "common.h"

typedef struct {
    struct sockaddr *addr;
    boolean internal;
    boolean tcp;
    uint64_t expected_response_time;
    uint64_t expected_fake_response_time;
} upstream_proxy_t;

typedef struct query_task_t query_task_t;
typedef struct session_ctx_t session_ctx_t;

typedef void(*task_cb)(query_task_t *task, char *response, ssize_t len, uint64_t response_time);

struct query_task_t {
    upstream_proxy_t *proxy;
    uint8_t response_count;
    char *msg;
    ssize_t msg_len;
    task_cb cb;
    uint64_t start_time;
    uv_handle_t *handle;
    session_ctx_t *ctx;
};


void task_init(query_task_t *task, session_ctx_t * ctx, upstream_proxy_t *proxy, char *msg, ssize_t len, task_cb cb);

void task_run(uv_loop_t *loop, query_task_t *task);

void task_close(query_task_t *task);

#endif //GDNS_PROXY_H
