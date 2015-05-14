#include <string.h>
#include <arpa/nameser.h>
#include <stdbool.h>
#include "session.h"
#include "common.h"
#include "task.h"
#include "server.h"

static void session_close(session_ctx_t *ctx);

static void on_task_done(query_task_t *task, char *response, ssize_t len, uint64_t response_time);

static void on_query_timeout(uv_timer_t *handle);

static void write_response(session_ctx_t *ctx, char *response, ssize_t len);

void session_setup(uv_udp_t *handle, const struct sockaddr *client_addr, char *data, ssize_t len,
                   upstream_proxy_t *proxys, int proxy_count, int query_timeout) {
    int i = 0;

    // initial session
    session_ctx_t *ctx = TMALLOC(session_ctx_t);
    memcpy(&(ctx->client_addr), client_addr, sizeof(struct sockaddr));

    ctx->query_data = xmalloc(len);
    memcpy(ctx->query_data, data, len);
    ctx->query_len = len;

    ctx->query_timeout = query_timeout;

    uv_timer_init(handle->loop, &ctx->timer);
    ctx->timer.data = ctx;

    ctx->finished = false;
    ctx->server_handle = handle;
    ctx->max_confidence = 0.0;
    ctx->confident_response = NULL;
    ctx->confident_response_len = 0;

    // start task
    ctx->tasks = xmalloc(sizeof(query_task_t) * proxy_count);
    ctx->task_count = proxy_count;

    for (i = 0; i < proxy_count; ++i) {
        task_init(&(ctx->tasks[i]), ctx, &(proxys[i]), ctx->query_data, ctx->query_len, on_task_done);
    }

    for (i = 0; i < proxy_count; ++i) {
        task_run(handle->loop, &(ctx->tasks[i]));
    }
    uv_timer_start(&ctx->timer, on_query_timeout, query_timeout, 0);
}


static void session_close(session_ctx_t *ctx) {
    int i = 0;

    for (i = 0; i < ctx->task_count; ++i) {
        task_close(&(ctx->tasks[i]));
    }

    uv_close((uv_handle_t *) &ctx->timer, NULL);
    xfree(ctx->query_data);
    xfree(ctx->tasks);
    xfree(ctx);
}

static void on_query_timeout(uv_timer_t *handle) {
    session_ctx_t *ctx = handle->data;
    if (!ctx->finished) {
        if (ctx->confident_response != NULL) {
            write_response(ctx, ctx->confident_response, ctx->confident_response_len);
            xfree(ctx->confident_response);
        } else {
            // just close session
            session_close(handle->data);
        }
    }
}

static int should_forward(query_task_t *task, char *response, ssize_t len, uint64_t response_time) {
    log_info("%ld",response_time);
    // 1 forward. 0 ignore.
    upstream_proxy_t *proxy = task->proxy;
    ns_msg msg;
    ns_rr rr;
    ns_type rr_type;
    int rr_count, i;

    // 1. forward tcp result.
    if (proxy->tcp) {
        return 1;
    }

    ns_initparse(response, len, &msg);
    rr_count = ns_msg_count(msg, ns_s_an);

    // 2. fake result have A record.
    if(rr_count == 0){
        return 1;
    }

    for (i = 0; i < rr_count; ++i) {
        if (ns_parserr(&msg, ns_s_an, i, &rr)) {
            log_error("error in parse resource record.");
            continue;
        }

        rr_type = ns_rr_type(rr);

        // 3. fake result will only have one type A answer.
        if (rr_type != ns_t_a) {
            return 1;
        }

        // 4. internal ip is reliable.
        server_ctx_t *server_ctx = task->ctx->server_handle->loop->data;
        if (ip_in_subnet_list(&server_ctx->list, (struct in_addr *) (ns_rr_rdata(rr)))) {
            return 1;
        }

        if (proxy->internal) { // external ip using external dns server only.
            return 0;
        }

        // 5. for external ip. calc result confidence.
        double confidence = (double) (response_time - task->proxy->expected_fake_response_time) /
                            (task->proxy->expected_response_time - task->proxy->expected_fake_response_time);
        log_info("confidence: %lf", confidence);
        // we are confident.
        if (confidence > 0.8) {
            return 1;
        }

        // else update the max confident response.
        if (confidence > task->ctx->max_confidence) {
            task->ctx->max_confidence = confidence;
            if (task->ctx->confident_response != NULL) {
                xfree(task->ctx->confident_response);
            }
            task->ctx->confident_response = xmalloc(len);
            memcpy(task->ctx->confident_response, response, len);
            task->ctx->confident_response_len = len;
        }
        break;
    }

    return 0;

}


static void on_task_done(query_task_t *task, char *response, ssize_t len, uint64_t response_time) {
    if (!task->ctx->finished && should_forward(task, response, len, response_time)) {
        task->ctx->finished = true;
        write_response(task->ctx, response, len);
    }
}

static void on_send_query_response(uv_udp_send_t *req, int status) {
    if (status != 0) {
        log_error("Error on send udp response: %s", uv_strerror(status));
    } else {
        // OK close session;
        session_close(req->data);
    }
    xfree(req);
}

static void write_response(session_ctx_t *ctx, char *response, ssize_t len) {
    char *data = xmalloc(len);
    memcpy(data, response, len);

    send_req_t *req = TMALLOC(send_req_t);

    req->buf = uv_buf_init(data, len);
    req->req.data = ctx;
    uv_udp_send((uv_udp_send_t *) req, ctx->server_handle, &req->buf, 1, &ctx->client_addr, on_send_query_response);

}