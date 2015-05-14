#include "server.h"
#include "task.h"

int main()
{
    struct sockaddr_in bind_address;
    struct sockaddr_in p_addr1;
    struct sockaddr_in p_addr2;

    server_cfg_t cfg;
    cfg.subnet_file_path = "subnets.txt";
    uv_ip4_addr("127.0.0.1", 5555, &bind_address);
    cfg.bind_address = (struct sockaddr *) &bind_address;

    cfg.proxy_count = 2;
    cfg.proxys = xmalloc(sizeof(upstream_proxy_t) * 2);

    uv_ip4_addr("8.8.4.4",53,&p_addr1);

    cfg.proxys[0].addr = (struct sockaddr *) &p_addr1;
    cfg.proxys[0].internal = false;
    cfg.proxys[0].tcp = false;
    cfg.proxys[0].expected_fake_response_time = 30;
    cfg.proxys[0].expected_response_time = 90;

    uv_ip4_addr("114.212.11.66",53,&p_addr2);

    cfg.proxys[1].addr = (struct sockaddr *) &p_addr2;
    cfg.proxys[1].internal = true;
    cfg.proxys[1].tcp = false;

    cfg.query_timeout = 2000 * 100;

    run_server(uv_default_loop(), &cfg);

}
