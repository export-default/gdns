#ifndef GDNS_COMMON_H
#define GDNS_COMMON_H

#include <stdint.h>
#include <uv.h>
#include <assert.h>

#define true 1
#define false 0
typedef uint8_t boolean;

typedef struct {
    uv_write_t req;
    uv_buf_t buf;
} write_req_t;

typedef struct {
    uv_udp_send_t req;
    uv_buf_t buf;
} send_req_t;

void log_info(const char *fmt, ...);

void log_warn(const char *fmt, ...);

void log_error(const char *fmt, ...);

void *xmalloc(ssize_t size);

void xfree(void *ptr);

void alloc_cb(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);

#define TMALLOC(TYPE) (TYPE *)xmalloc(sizeof(TYPE))

#define UNREACHABLE() assert(!"Unreachable code reached.")


#endif //GDNS_COMMON_H
