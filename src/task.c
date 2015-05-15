#include "task.h"
#include "common.h"

static void run_udp_task(uv_loop_t *loop, query_task_t *task);

static void run_tcp_task(uv_loop_t *loop, query_task_t *task);

static void on_send_udp_query(uv_udp_send_t *req, int status);

static query_task_t *get_task(uv_handle_t *handle);

static void on_recv_udp_response(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf,
                                 const struct sockaddr *addr, unsigned flags);

static void on_write_tcp_query(uv_write_t *req, int status);

static void on_read_tcp_response(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);

static void on_close(uv_handle_t * handle);

void task_init(query_task_t *task, session_ctx_t *ctx, upstream_proxy_t *proxy, char *msg, ssize_t len, task_cb cb) {
    task->proxy = proxy;
    task->cb = cb;
    task->start_time = 0;
    task->msg = msg;
    task->msg_len = len;
    task->ctx = ctx;
}

void task_run(uv_loop_t *loop, query_task_t *task) {
    if (task->proxy->tcp) {
        run_tcp_task(loop, task);
    } else {
        run_udp_task(loop, task);
    }
    task->start_time = uv_hrtime();
}

void task_close(query_task_t *task) {
    if (task->proxy->tcp) {
        uv_read_stop((uv_stream_t *) task->handle);
    } else {
        uv_udp_recv_stop((uv_udp_t *) task->handle);
    }
    uv_close(task->handle, on_close);
}

static void run_udp_task(uv_loop_t *loop, query_task_t *task) {
    uv_udp_t *handle = TMALLOC(uv_udp_t);

    uv_udp_init(loop, handle);
    handle->data = task;
    task->handle = (uv_handle_t *) handle;


    send_req_t *req = TMALLOC(send_req_t);
    req->buf = uv_buf_init(task->msg, (unsigned int) task->msg_len);
    uv_udp_send((uv_udp_send_t *) req, handle, &req->buf, 1, task->proxy->addr, on_send_udp_query);
}

static void run_tcp_task(uv_loop_t *loop, query_task_t *task) {
    uv_tcp_t *handle = TMALLOC(uv_tcp_t);

    uv_tcp_init(loop, handle);
    handle->data = task;
    task->handle = (uv_handle_t *) handle;

    write_req_t *req = TMALLOC(write_req_t);
    req->buf = uv_buf_init(task->msg, (unsigned int) task->msg_len);

    uv_write((uv_write_t *) req, (uv_stream_t *) handle, &req->buf, 1, on_write_tcp_query);
}

static query_task_t *get_task(uv_handle_t *handle) {
    return (query_task_t *) handle->data;
}


static void on_send_udp_query(uv_udp_send_t *req, int status) {
    if (status != 0) {
        log_error("Error on forward udp query: %s", uv_strerror(status));
    } else {
        uv_udp_recv_start(req->handle, alloc_cb, on_recv_udp_response);
    }
    xfree(req);
}

static void on_recv_udp_response(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf,
                                 const struct sockaddr *addr, unsigned flags) {
    if (nread < 0) {
        log_error("Error on read udp proxy response: %s", uv_strerror((int) nread));
    } else if (nread > 0) {
        query_task_t *task = get_task((uv_handle_t *) handle);
        task->cb(task, buf->base, nread, (uint64_t) ((uv_hrtime() - task->start_time) / 1e6));
    }

    if (buf->base)
        xfree(buf->base);
}

static void on_write_tcp_query(uv_write_t *req, int status) {
    if (status != 0) {
        log_error("Error on forward tcp query: %s", uv_strerror(status));
    } else {
        uv_read_start(req->handle, alloc_cb, on_read_tcp_response);
    }
    xfree(req);
}

static void on_read_tcp_response(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
    if (nread < 0) {
        log_error("Error on read tcp proxy response: %s", uv_strerror((int) nread));
    } else if (nread > 0) {
        query_task_t *task = get_task((uv_handle_t *) stream);
        task->cb(task, buf->base, nread, (uint64_t) ((uv_hrtime() - task->start_time) / 1e6));
    }

    if (buf->base)
        xfree(buf->base);
}

static void on_close(uv_handle_t * handle)
{
    xfree(handle);
}