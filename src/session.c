#include <string.h>
#include <arpa/nameser.h>
#include <stdbool.h>
#include "session.h"
#include "common.h"
#include "task.h"
#include "server.h"
#include "iputility.h"

static void session_close(session_ctx_t *ctx);

static void on_task_done(query_task_t *task, char *response, ssize_t len, int64_t response_time);

static void on_query_timeout(uv_timer_t *handle);

static void write_response(session_ctx_t *ctx, char *response, ssize_t len);

static void on_close(uv_handle_t *handle);

static int forward_action(query_task_t *task, char *response, ssize_t len, int64_t response_time);

static void on_task_close(query_task_t *task);

static void on_timer_close(uv_handle_t *handle);

void session_setup(server_ctx_t *server_ctx, const struct sockaddr *client_addr, char *data, ssize_t len,
                   upstream_proxy_t *proxys, int proxy_count, int query_timeout) {
    int i = 0;

    // initial session
    session_ctx_t *ctx = TMALLOC(session_ctx_t);
    memcpy(&(ctx->client_addr), client_addr, sizeof(struct sockaddr));

    ctx->query_data = xmalloc(len);
    memcpy(ctx->query_data, data, len);
    ctx->query_len = len;

    ctx->query_timeout = query_timeout;

    ctx->timer = TMALLOC(uv_timer_t);
    uv_timer_init(server_ctx->handle->loop, ctx->timer);
    ctx->timer->data = ctx;

    ctx->server_ctx = server_ctx;

    ctx->max_confidence = 0.0;
    ctx->confident_response = NULL;
    ctx->confident_response_len = 0;

    // start task
    ctx->tasks = xmalloc(sizeof(query_task_t *) * proxy_count);
    ctx->task_count = proxy_count;

    for (i = 0; i < proxy_count; ++i) {
        ctx->tasks[i] = TMALLOC(query_task_t);
        task_init(ctx->tasks[i], &(proxys[i]), ctx->query_data, ctx->query_len);
        ctx->tasks[i]->data = ctx;
    }

    for (i = 0; i < proxy_count; ++i) {
        task_run(server_ctx->handle->loop, ctx->tasks[i], on_task_done);
    }

    uv_timer_start(ctx->timer, on_query_timeout, (uint64_t) ctx->query_timeout, 0);
    ctx->state = SESSION_RUNNING;
}


static void session_close(session_ctx_t *ctx) {
    int i = 0;
    for (i = 0; i < ctx->task_count; ++i) {
        task_close(ctx->tasks[i], on_task_close);
    }
    uv_close((uv_handle_t *) ctx->timer, on_timer_close);

    xfree(ctx->query_data);
    xfree(ctx->tasks);
    if (ctx->confident_response) {
        xfree(ctx->confident_response);
    }
    xfree(ctx);
}

static void on_query_timeout(uv_timer_t *handle) {
    session_ctx_t *ctx = handle->data;
    uv_timer_stop(handle);

    if (ctx->state == SESSION_RUNNING) { // still running.
        if (ctx->confident_response != NULL) {
            write_response(ctx, ctx->confident_response, ctx->confident_response_len);
        } else {
            // just close session
            session_close(ctx);
        }
    }

}

static void on_task_done(query_task_t *task, char *response, ssize_t len, int64_t response_time) {
    if (task->state == TASK_DONE) {
        session_ctx_t *ctx = task->data;
        if (ctx->state == SESSION_RUNNING && forward_action(task, response, len, response_time) == 1) {
            uv_timer_stop(ctx->timer);
            ctx->state = SESSION_DONE;
            write_response(ctx, response, len);
        }
    }
}

static void on_send_query_response(uv_udp_send_t *req, int status) {
    session_ctx_t * ctx = req->data;
    if (status != 0) {
        log_error("Error on send udp response: %s", uv_strerror(status));
        uv_timer_stop(ctx->timer);
    }
    session_close(ctx);
    xfree(req);
}

static void write_response(session_ctx_t *ctx, char *response, ssize_t len) {
    char *data = xmalloc(len);
    memcpy(data, response, len);

    send_req_t *req = TMALLOC(send_req_t);

    req->buf = uv_buf_init(data, len);
    req->req.data = ctx;
    uv_udp_send((uv_udp_send_t *) req, ctx->server_ctx->handle, &req->buf, 1, &ctx->client_addr,
                on_send_query_response);

}


static void on_task_close(query_task_t *task) {
    xfree(task);
}

static void on_timer_close(uv_handle_t *handle) {
    xfree(handle);
}


// 1 forward. 0 ignore.
static int forward_action(query_task_t *task, char *response, ssize_t len, int64_t response_time) {

    upstream_proxy_t *proxy = task->proxy;
    session_ctx_t *ctx = task->data;
    ns_msg msg;
    ns_rr rr;
    ns_type rr_type;
    int rr_count, i;

    // 1. forward tcp result.
    if(task->proxy->tcp){
        return 1;
    }

    ns_initparse(response, len, &msg);
    rr_count = ns_msg_count(msg, ns_s_an);

    // 2. fake result will have A record.
    if (rr_count == 0) {
        return 1;
    }

    for (i = 0; i < rr_count; ++i) {
        if (ns_parserr(&msg, ns_s_an, i, &rr)) {
            log_error("error in parse resource record.");
            continue;
        }

        rr_type = ns_rr_type(rr);

        // 3. fake result will only have one A record.
        if (rr_type != ns_t_a) {
            return 1;
        }

        // 4. internal ip is reliable.
        server_ctx_t *server_ctx = ctx->server_ctx;
        if (ip_in_subnet_list(&server_ctx->list, (struct in_addr *) (ns_rr_rdata(rr)))) {
            return 1;
        }

        if (proxy->internal) { // external ip using external dns server only.
            return 0;
        }

        // 5. for external ip. calc result confidence.
        double confidence = ( 1.0 * response_time - proxy->expected_fake_response_time) /
                            (proxy->expected_response_time - proxy->expected_fake_response_time);

        // if we are confident enough.
        if (confidence > 0.8) {
            return 1;
        }

        // else update the max confident response in case of timeout.
        if (confidence > ctx->max_confidence) {
            ctx->max_confidence = confidence;
            if (ctx->confident_response != NULL) {
                xfree(ctx->confident_response);
            }
            ctx->confident_response = xmalloc(len);
            memcpy(ctx->confident_response, response, len);
            ctx->confident_response_len = len;
        }
        break;
    }

    return 0;

}