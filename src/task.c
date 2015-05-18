#include <string.h>
#include "task.h"

static void run_udp_task(uv_loop_t *loop, query_task_t *task);

static void run_tcp_task(uv_loop_t *loop, query_task_t *task);

static void on_send_udp_query(uv_udp_send_t *req, int status);

static void bind_task_to_handle(query_task_t *task, uv_handle_t *handle);

static query_task_t *get_task_from_handle(uv_handle_t *handle);

static void on_recv_udp_response(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf,
                                 const struct sockaddr *addr, unsigned flags);

static void on_write_tcp_query(uv_write_t *req, int status);

static void on_read_tcp_response(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);

static int64_t time_diff(uint64_t start_time);

static void on_close(uv_handle_t *handle);

void task_init(query_task_t *task, upstream_proxy_t *proxy, char *msg, ssize_t len) {
    task->proxy = proxy;
    task->start_time = 0;
    if (proxy->tcp) {
        // tcp request add 2 bytes data length in header.
        task->msg = xmalloc(len + 2);
        *((uint16_t *) task->msg) = htons(len);
        memcpy(task->msg + 2, msg, len);
        task->msg_len = len + 2;
    } else {
        task->msg = xmalloc(len);
        memcpy(task->msg, msg, len);
        task->msg_len = len;
    }
    task->state = TASK_INIT;
}

void task_run(uv_loop_t *loop, query_task_t *task, task_cb cb) {
    task->cb = cb;
    if (task->proxy->tcp) {
        run_tcp_task(loop, task);
    } else {
        run_udp_task(loop, task);
    }
    task->start_time = uv_hrtime();
    task->state = TASK_RUNING;
}

void task_close(query_task_t *task, task_close_cb close_cb) {
    task->close_cb = close_cb;
    uv_close(task->handle, on_close);
}

static void run_udp_task(uv_loop_t *loop, query_task_t *task) {
    uv_udp_t *handle = TMALLOC(uv_udp_t);

    uv_udp_init(loop, handle);
    task->handle = (uv_handle_t *) handle;
    bind_task_to_handle(task, task->handle);

    send_req_t *req = TMALLOC(send_req_t);
    req->buf = uv_buf_init(task->msg, (unsigned int) task->msg_len);
    uv_udp_send((uv_udp_send_t *) req, handle, &req->buf, 1, task->proxy->addr, on_send_udp_query);
}

static void on_tcp_connect(uv_connect_t *req, int status) {
    query_task_t *task = get_task_from_handle((uv_handle_t *) req->handle);
    if (status != 0) {
        log_error("Error when connecting to remote: %s", uv_strerror(status));
        task->state = TASK_ERROR;
        task->cb(task, NULL, 0, 0);
    }
    else {
        write_req_t *write_req = TMALLOC(write_req_t);
        write_req->buf = uv_buf_init(task->msg, (unsigned int) task->msg_len);

        uv_write((uv_write_t *) write_req, req->handle, &write_req->buf, 1, on_write_tcp_query);
    }
    xfree(req);
}

static void run_tcp_task(uv_loop_t *loop, query_task_t *task) {
    uv_tcp_t *handle = TMALLOC(uv_tcp_t);

    uv_tcp_init(loop, handle);
    task->handle = (uv_handle_t *) handle;
    bind_task_to_handle(task, task->handle);

    uv_connect_t *req = TMALLOC(uv_connect_t);
    uv_tcp_connect(req, handle, task->proxy->addr, on_tcp_connect);
}

static void bind_task_to_handle(query_task_t *task, uv_handle_t *handle) {
    handle->data = task;
}

static query_task_t *get_task_from_handle(uv_handle_t *handle) {
    return (query_task_t *) handle->data;
}


static void on_send_udp_query(uv_udp_send_t *req, int status) {
    query_task_t *task = get_task_from_handle((uv_handle_t *) req->handle);
    if (status != 0) {
        log_error("Error on forward udp query: %s", uv_strerror(status));
        task->state = TASK_ERROR;
        task->cb(task, NULL, 0, 0);
    } else {
        uv_udp_recv_start(req->handle, alloc_cb, on_recv_udp_response);
    }
    xfree(req);
}

static void on_recv_udp_response(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf,
                                 const struct sockaddr *addr, unsigned flags) {
    query_task_t *task = get_task_from_handle((uv_handle_t *) handle);
    if (nread < 0) {
        log_error("Error on read udp proxy response: %s", uv_strerror((int) nread));
        task->state = TASK_ERROR;
        task->cb(task, NULL, 0, 0);
    } else if (nread > 0) {
        if (task->state == TASK_RUNING) {
            task->state = TASK_DONE;
        } else if (task->state == TASK_DONE || task->state == TASK_MULTI_RESULT) {
            task->state = TASK_MULTI_RESULT;
        } else {
            UNREACHABLE();
        }
        task->cb(task, buf->base, nread, time_diff(task->start_time));
    }

    if (buf->base)
        xfree(buf->base);
}

static void on_write_tcp_query(uv_write_t *req, int status) {
    query_task_t *task = get_task_from_handle((uv_handle_t *) req->handle);
    if (status != 0) {
        log_error("Error on forward tcp query: %s", uv_strerror(status));
        task->state = TASK_ERROR;
        task->cb(task, NULL, 0, 0);
    } else {
        uv_read_start(req->handle, alloc_cb, on_read_tcp_response);
    }
    xfree(req);
}

static void on_read_tcp_response(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
    query_task_t *task = get_task_from_handle((uv_handle_t *) stream);
    if (nread < 0) {
        log_error("Error on read tcp proxy response: %s", uv_strerror((int) nread));
        task->state = TASK_ERROR;
        task->cb(task, NULL, 0, 0);
    } else if (nread > 0) {
        if (task->state == TASK_RUNING) {
            task->state = TASK_DONE;
        } else if (task->state == TASK_DONE || task->state == TASK_MULTI_RESULT) {
            task->state = TASK_MULTI_RESULT;
        } else {
            UNREACHABLE();
        }
        task->cb(task, buf->base + 2, nread - 2,
                 time_diff(task->start_time)); // tcp response add 2 bytes data length in header.
        uv_read_stop(stream);
    }

    if (buf->base)
        xfree(buf->base);
}

static void on_close(uv_handle_t *handle) {
    query_task_t *task = get_task_from_handle(handle);
    xfree(handle);
    xfree(task->msg);
    if (task->close_cb) {
        task->close_cb(task);
    }
}

static int64_t time_diff(uint64_t start_time) {
    return (int64_t) ((uv_hrtime() - start_time) / 1e6);
}