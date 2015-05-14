#ifndef GDNS_SESSION_H
#define GDNS_SESSION_H

#include "common.h"
#include "task.h"

struct session_ctx_t{
    struct sockaddr client_addr;
    char * query_data;
    ssize_t query_len;
    query_task_t * tasks;
    int task_count;
    int query_timeout;
    uv_timer_t timer;
    boolean finished;
    uv_udp_t * server_handle;
    double max_confidence;
    char * confident_response;
    ssize_t confident_response_len;
} ;

void session_setup(uv_udp_t * handle, const struct sockaddr * client_addr, char * data, ssize_t len,
                   upstream_proxy_t * proxys, int proxy_count, int query_timeout);

#endif //GDNS_SESSION_H
