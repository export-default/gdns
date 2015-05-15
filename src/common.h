#ifndef GDNS_COMMON_H
#define GDNS_COMMON_H

#include <stdint.h>
#include <uv.h>
#include <assert.h>
#include <stdbool.h>

/*
 * Type defs.
 */

typedef struct {
    uv_write_t req;
    uv_buf_t buf;
} write_req_t;

typedef struct {
    uv_udp_send_t req;
    uv_buf_t buf;
} send_req_t;

typedef struct {
    in_addr_t addr;
    in_addr_t mask;
} subnet_t;

typedef struct {
    int len;
    subnet_t *subnets;
} subnet_list_t;

typedef struct {
    struct sockaddr *addr;
    bool internal;
    bool tcp;
    bool enabled;
    int64_t expected_response_time;
    int64_t expected_fake_response_time;
    void *data;
} upstream_proxy_t;

typedef struct {
    struct sockaddr *bind_address;
    upstream_proxy_t *proxies;
    int proxies_count;
    int query_timeout; // ms
    char *subnet_file_path;
    char **blocked_domain;
    int blocked_domain_len;
    char **non_blocked_domain;
    int non_blocked_domain_len;
    bool verbose;
} server_cfg_t;

typedef struct {
    server_cfg_t *cfg;
    uv_udp_t *handle;
    subnet_list_t list;
} server_ctx_t;


typedef struct query_task_t query_task_t;

typedef enum {
    SESSION_RUNNING,
    SESSION_DONE
} session_state_t;

typedef struct {
    struct sockaddr client_addr;
    char *query_data;
    ssize_t query_len;
    query_task_t **tasks;
    int task_count;
    int query_timeout;
    uv_timer_t *timer;
    server_ctx_t *server_ctx;
    double max_confidence;
    char *confident_response;
    ssize_t confident_response_len;
    session_state_t state;
} session_ctx_t;

typedef void(*task_cb)(query_task_t *task, char *response, ssize_t len, int64_t response_time);

typedef void(*task_close_cb)(query_task_t *task);

typedef enum {
    TASK_INIT,
    TASK_RUNING,
    TASK_DONE,
    TASK_ERROR,
    TASK_MULTI_RESULT
} query_task_state_t;

struct query_task_t {
    upstream_proxy_t *proxy;
    char *msg;
    ssize_t msg_len;
    task_cb cb;
    query_task_state_t state;
    uint64_t start_time;
    uv_handle_t *handle;
    task_close_cb close_cb;
    void *data;
};

/**
 * logging utilities
 */

void log_info(const char *fmt, ...);

void log_warn(const char *fmt, ...);

void log_error(const char *fmt, ...);

/**
 * memory utilities
 */

void *xmalloc(ssize_t size);

void xfree(void *ptr);

void alloc_cb(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);

#define TMALLOC(TYPE) (TYPE *)xmalloc(sizeof(TYPE))


/**
 * Others.
 */

#define UNREACHABLE() assert(!"Unreachable code reached.")


#endif //GDNS_COMMON_H
