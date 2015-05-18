#include "server.h"
#include "session.h"
#include "iputility.h"
#include "common.h"

#include "proxy.h"

static void on_read_dns_query(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr,
                              unsigned flags);

static void server_close(server_ctx_t *ctx);

static void on_close(uv_handle_t *handle);

static void on_proxies_init(server_ctx_t * ctx);

int run_server(uv_loop_t *loop, server_cfg_t *cfg) {
    server_ctx_t *ctx = TMALLOC(server_ctx_t);
    uv_udp_t *handle = TMALLOC(uv_udp_t);
    int rv;

    ctx->cfg = cfg;
    loop->data = ctx;
    ctx->handle = handle;

    if (subnet_list_init(cfg->subnet_file_path, &ctx->list)) {
        log_error("parse subnet file failed!");
        return 1;
    }

    uv_udp_init(loop, handle);

    if ((rv = uv_udp_bind(handle, cfg->bind_address, 0)) != 0) {
        log_error("bind failed! %s", uv_strerror(rv));
        uv_close((uv_handle_t *) handle, NULL);
        return 1;
    } else {

        proxies_init(ctx, loop, on_proxies_init); // init proxy's expected_xx_time.

        rv = uv_run(loop, UV_RUN_DEFAULT);
        server_close(ctx);
        return rv;
    }

}

static void on_read_dns_query(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr,
                              unsigned flags) {
    if (nread < 0) {
        log_error("error read client dns query. %s", uv_strerror((int) nread));
    } else if (nread > 0) {
        server_ctx_t *ctx = handle->loop->data;
        server_cfg_t *cfg = ctx->cfg;
        session_setup(ctx, addr, buf->base, nread, cfg->proxies, cfg->proxies_count, cfg->query_timeout);
    }
    if (buf->base)
        xfree(buf->base);
}

static void server_close(server_ctx_t *ctx) {
    uv_close((uv_handle_t *) ctx->handle, on_close);
}

static void on_close(uv_handle_t *handle) {
    server_ctx_t *ctx = handle->loop->data;
    subnet_list_free(&ctx->list);
    xfree(ctx->handle);
    xfree(ctx);
}

static void on_proxies_init(server_ctx_t * ctx)
{
    uv_udp_recv_start(ctx->handle, alloc_cb, on_read_dns_query); // starting service
}
