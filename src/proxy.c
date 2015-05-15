#include "proxy.h"
#include "common.h"
#include "task.h"
#include <resolv.h>


typedef struct {
    uv_timer_t *timer;
    query_task_t **tasks;
    int tasks_len;
    server_ctx_t *server_ctx;
    proxies_init_cb cb;
} pinit_ctx_t;

typedef struct {
    int blocked_count;
    int non_blocked_count;
    uint64_t blocked_time;
    uint64_t non_blocked_time;
} pinit_req_t;

static void init_req(pinit_req_t *req, upstream_proxy_t *proxy) {
    req->blocked_count = 0;
    req->non_blocked_count = 0;
    req->blocked_time = 0;
    req->non_blocked_time = 0;
}

static void on_task_close(query_task_t *task) {
    xfree(task);
}

static void on_timer_close(uv_handle_t *timer) {
    xfree(timer);
}

static void on_timeout(uv_timer_t *timer) {
    pinit_ctx_t *pctx = timer->data;
    server_ctx_t *ctx = pctx->server_ctx;
    server_cfg_t *cfg = ctx->cfg;
    int i;
    int domains_len = cfg->non_blocked_domain_len + cfg->blocked_domain_len;

    uv_timer_stop(timer);
    uv_close((uv_handle_t *) timer, on_timer_close);

    for (i = 0; i < pctx->tasks_len; ++i) {
        task_close(pctx->tasks[i], on_task_close);
    }
    for (i = 0; i < cfg->proxies_count; ++i) {
        upstream_proxy_t *proxy = &cfg->proxies[i];
        pinit_req_t *req = proxy->data;

        if (req->blocked_count + req->non_blocked_count < 0.8 * (domains_len)) {
            proxy->enabled = false;
        } else {
            proxy->enabled = true;
            proxy->expected_response_time = req->non_blocked_time / req->non_blocked_count;
            proxy->expected_fake_response_time = req->blocked_time / req->blocked_count;

            log_info("proxy[%d] - %d/%d %d %d", i, req->blocked_count + req->non_blocked_count, domains_len,
                     proxy->expected_fake_response_time, proxy->expected_response_time);
        }
        xfree(req);
    }
    xfree(pctx->tasks);

    if (pctx->cb) {
        pctx->cb(ctx);
    }
}

static void on_blocked_done(query_task_t *task, char *response, ssize_t len, int64_t response_time) {
    if (task->state == TASK_DONE) {
        pinit_req_t *req = task->proxy->data;
        req->blocked_count += 1;
        req->blocked_time += response_time;
    }
}

static void on_non_blocked_done(query_task_t *task, char *response, ssize_t len, int64_t response_time) {
    if (task->state == TASK_DONE) {
        pinit_req_t *req = task->proxy->data;
        req->non_blocked_count += 1;
        req->non_blocked_time += response_time;
    }
}

static void mkquery(char *domain, char **msg, ssize_t *len) {
    static char buf[PACKETSZ];
    *len = res_mkquery(QUERY, domain, C_IN, T_A, NULL, 0, NULL, buf, PACKETSZ);
    *msg = buf;
}

static void create_task(query_task_t *task, upstream_proxy_t *proxy, char *domain) {
    char *msg;
    ssize_t len;
    mkquery(domain, &msg, &len);
    task_init(task, proxy, msg, len);
}

void proxies_init(server_ctx_t *ctx, uv_loop_t *loop, proxies_init_cb cb) {
    int init_timeout = 3000; //ms
    int i, j, k;
    int len;
    int index = 0;
    int current = 0;
    server_cfg_t *cfg = ctx->cfg;

    pinit_ctx_t *pctx = TMALLOC(pinit_ctx_t);
    pctx->server_ctx = ctx;
    pctx->cb = cb;
    pctx->timer = TMALLOC(uv_timer_t);
    pctx->timer->data = pctx;

    uv_timer_init(loop, pctx->timer);

    len = (cfg->non_blocked_domain_len + cfg->blocked_domain_len) * cfg->proxies_count;

    pctx->tasks = xmalloc(sizeof(query_task_t *) * len);
    pctx->tasks_len = len;

    for (i = 0; i < cfg->proxies_count; ++i) {
        upstream_proxy_t *proxy = &cfg->proxies[i];
        pinit_req_t *req = TMALLOC(pinit_req_t);
        init_req(req, proxy);
        proxy->data = req;

        for (j = 0; j < cfg->blocked_domain_len; ++j) {
            current = index++;
            pctx->tasks[current] = TMALLOC(query_task_t);
            create_task(pctx->tasks[current], proxy, cfg->blocked_domain[j]);
            task_run(loop, pctx->tasks[current], on_blocked_done);
        }

        for (k = 0; k < cfg->non_blocked_domain_len; ++k) {
            current = index++;
            pctx->tasks[current] = TMALLOC(query_task_t);
            create_task(pctx->tasks[current], proxy, cfg->non_blocked_domain[k]);
            task_run(loop, pctx->tasks[current], on_non_blocked_done);
        }
    }

    uv_timer_start(pctx->timer, on_timeout, init_timeout, 0);
}