#ifndef GDNS_CONFIG_H
#define GDNS_CONFIG_H

#include "common.h"

server_cfg_t * read_server_cfg(const char * filepath);

void free_server_cfg(server_cfg_t *);

#endif //GDNS_CONFIG_H
