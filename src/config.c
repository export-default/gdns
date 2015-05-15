#include "config.h"
#include "common.h"
#include <libconfig.h>
#include <string.h>
#include <stdlib.h>


static void ensure_true(int rv, config_t *cfg);

server_cfg_t *read_server_cfg(const char *filepath) {
    config_t config;
    config_setting_t *settings;
    server_cfg_t *server_cfg;
    int rv;
    const char *ip;
    int port;
    int timeout;
    const char *subnets_file_path;
    int len;
    struct sockaddr_in *addr;
    int i;

    server_cfg = TMALLOC(server_cfg_t);
    config_init(&config);

    rv = config_read_file(&config, filepath);

    ensure_true(rv, &config);

    rv = config_lookup_string(&config, "server.ip", &ip);
    ensure_true(rv, &config);
    rv = config_lookup_int(&config, "server.port", &port);
    ensure_true(rv, &config);
    rv = config_lookup_int(&config, "server.timeout", &timeout);
    ensure_true(rv, &config);
    rv = config_lookup_string(&config, "server.subnets_file", &subnets_file_path);
    ensure_true(rv, &config);

    addr = TMALLOC(struct sockaddr_in);
    uv_ip4_addr(ip, port, addr);
    server_cfg->bind_address = (struct sockaddr *) addr;
    server_cfg->query_timeout = timeout;
    server_cfg->subnet_file_path = xmalloc(strlen(subnets_file_path) + 1);
    strcpy(server_cfg->subnet_file_path, subnets_file_path);

    settings = config_lookup(&config, "server.proxies");
    len = config_setting_length(settings);

    if (settings == NULL || len == 0) {
        log_error("there is no proxy to use.");
        exit(-1);
    }

    server_cfg->proxies_count = len;
    server_cfg->proxies = xmalloc(sizeof(upstream_proxy_t) * len);

    for (i = 0; i < len; ++i) {
        config_setting_t *proxy;
        const char *proxy_ip;
        int proxy_port;
        int internal;
        int tcp;
        proxy = config_setting_get_elem(settings, i);
        rv = config_setting_lookup_string(proxy, "ip", &proxy_ip);
        ensure_true(rv, &config);
        rv = config_setting_lookup_int(proxy, "port", &proxy_port);
        ensure_true(rv, &config);
        rv = config_setting_lookup_bool(proxy, "internal", &internal);
        ensure_true(rv, &config);
        rv = config_setting_lookup_bool(proxy, "tcp", &tcp);
        ensure_true(rv, &config);

        addr = TMALLOC(struct sockaddr_in);
        uv_ip4_addr(proxy_ip, proxy_port, addr);
        server_cfg->proxies[i].addr = (struct sockaddr *) addr;
        server_cfg->proxies[i].internal = (bool) internal;
        server_cfg->proxies[i].tcp = (bool) tcp;
        server_cfg->proxies[i].expected_response_time = 90;
        server_cfg->proxies[i].expected_fake_response_time = 30;
    }

    settings = config_lookup(&config, "domains.blocked");
    len = config_setting_length(settings);

    if (settings == NULL || len == 0) {
        log_error("there is no blocked domain in configuration.");
        exit(-1);
    }
    server_cfg->blocked_domain_len = len;
    server_cfg->blocked_domain = xmalloc(sizeof(char *) * len);
    for (i = 0; i < len; ++i) {
        const char *domain = config_setting_get_string_elem(settings, 0);
        server_cfg->blocked_domain[i] = xmalloc(strlen(domain) + 1);
        strcpy(server_cfg->blocked_domain[i], domain);
    }

    settings = config_lookup(&config, "domains.non_blocked");
    len = config_setting_length(settings);

    if (settings == NULL || len == 0) {
        log_error("there is no non_blocked domain in configuration.");
        exit(-1);
    }
    server_cfg->non_blocked_domain_len = len;
    server_cfg->non_blocked_domain = xmalloc(sizeof(char *) * len);
    for (i = 0; i < len; ++i) {
        const char *domain = config_setting_get_string_elem(settings, 0);
        server_cfg->non_blocked_domain[i] = xmalloc(strlen(domain) + 1);
        strcpy(server_cfg->non_blocked_domain[i], domain);
    }


    config_destroy(&config);
    return server_cfg;
}

void free_server_cfg(server_cfg_t *cfg) {
    int i = 0;
    xfree(cfg->bind_address);
    xfree(cfg->subnet_file_path);
    for (i = 0; i < cfg->proxies_count; ++i) {
        xfree(cfg->proxies[i].addr);
    }
    for (i = 0; i < cfg->blocked_domain_len; ++i) {
        xfree(cfg->blocked_domain[i]);
    }
    for (i = 0; i < cfg->non_blocked_domain_len; ++i) {
        xfree(cfg->non_blocked_domain[i]);
    }
    xfree(cfg->blocked_domain);
    xfree(cfg->non_blocked_domain);
    xfree(cfg->proxies);
    xfree(cfg);
}

static void ensure_true(int rv, config_t *cfg) {
    if (rv != CONFIG_TRUE) {
        log_error("%s:%d - %s", config_error_file(cfg), config_error_line(cfg), config_error_text(cfg));
        exit(-1);
    }
}