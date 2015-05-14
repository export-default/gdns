#ifndef GDNS_SERVER_H
#define GDNS_SERVER_H

#include "common.h"
#include "task.h"
#include "iputility.h"


typedef struct {
    struct sockaddr *bind_address;
    upstream_proxy_t *proxys;
    int proxy_count;
    int query_timeout; // ms
    const char *subnet_file_path;
} server_cfg_t;

typedef struct {
    server_cfg_t *cfg;
    uv_udp_t *handle;
    subnet_list_t list;
} server_ctx_t;

int run_server(uv_loop_t *loop, server_cfg_t *cfg);

#endif //GDNS_SERVER_H
