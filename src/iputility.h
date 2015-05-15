#ifndef GDNS_IPUTILITY_H
#define GDNS_IPUTILITY_H

#include "common.h"

int subnet_list_init(const char *path, subnet_list_t *list);

void subnet_list_free(subnet_list_t *list);

bool ip_in_subnet_list(subnet_list_t *list, struct in_addr *addr);

#endif //GDNS_IPUTILITY_H
