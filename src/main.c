#include "server.h"
#include "config.h"


int main() {
    server_cfg_t *cfg = read_server_cfg("./gdns.conf");
    uv_loop_t * loop = uv_default_loop();
    run_server(loop, cfg);
    uv_loop_close(loop);
    free_server_cfg(cfg);
}
