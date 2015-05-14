#ifndef GDNS_IPUTILITY_H
#define GDNS_IPUTILITY_H

#include "common.h"

typedef struct {
    in_addr_t addr;
    in_addr_t mask;
} subnet_t;

typedef struct {
    int len;
    subnet_t *head;
} subnet_list_t;

int subnet_list_init(const char *path, subnet_list_t *list);

void subnet_list_free(subnet_list_t *list);

boolean ip_in_subnet_list(subnet_list_t *list, struct in_addr *addr);

#endif //GDNS_IPUTILITY_H
