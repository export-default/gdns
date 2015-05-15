#include "server.h"
#include "config.h"


int main(int argc, char **argv) {
    server_cfg_t *cfg = init_server_cfg(argc, argv);
    uv_loop_t *loop = uv_default_loop();
    run_server(loop, cfg);
    uv_loop_close(loop);
    free_server_cfg(cfg);
    return 0;
}
