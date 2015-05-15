#ifndef GDNS_CONFIG_H
#define GDNS_CONFIG_H

#include "common.h"

server_cfg_t * init_server_cfg(int argc, char ** argv);

void free_server_cfg(server_cfg_t *);

#endif //GDNS_CONFIG_H
