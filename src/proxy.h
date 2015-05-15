#ifndef GDNS_PROXY_H
#define GDNS_PROXY_H
#include "common.h"

typedef void(*proxies_init_cb)(server_ctx_t * ctx);

void proxies_init(server_ctx_t * ctx, uv_loop_t * loop, proxies_init_cb cb);

#endif //GDNS_PROXY_H
