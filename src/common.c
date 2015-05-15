#include "common.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

static void do_log(FILE *stream, const char *label, const char *fmt, va_list ap) {
    char fmtbuf[1024];
    vsnprintf(fmtbuf, sizeof(fmtbuf), fmt, ap);
    fprintf(stream, "%s: %s\n", label, fmtbuf);
}

void log_info(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    do_log(stdout, "info", fmt, ap);
    va_end(ap);
}

void log_warn(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    do_log(stderr, "warn", fmt, ap);
    va_end(ap);
}

void log_error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    do_log(stderr, "error", fmt, ap);
    va_end(ap);
}

void *xmalloc(ssize_t size) {
    void *ptr;

    ptr = malloc((size_t) size);
    if (ptr == NULL) {
        log_error("out of memory, need %lu bytes", (unsigned long) size);
        exit(1);
    }

    return ptr;
}

void xfree(void *ptr) {
    free(ptr);
}

void alloc_cb(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
    buf->base = xmalloc(suggested_size);
    buf->len = suggested_size;
}