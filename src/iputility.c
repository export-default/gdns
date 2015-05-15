#include "iputility.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


static int cmp_subnet(const void *s1, const void *s2);

int subnet_list_init(const char *path, subnet_list_t *list) {
    FILE *fp;
    struct in_addr addr;
    char buf[24];
    char *line;
    int i = 0;
    list->len = 0;

    fp = fopen(path, "rb");
    if (fp == NULL) {
        log_error("Can not open file %s", path);
        return -1;
    }

    while ((line = fgets(buf, sizeof(buf), fp))) {
        list->len += 1;
    }

    list->subnets = xmalloc(sizeof(subnet_t) * list->len);

    fseek(fp, 0, SEEK_SET);

    while ((line = fgets(buf, sizeof(buf), fp))) {
        char *delimiter;
        delimiter = strchr(line, '/');
        if (delimiter) {
            list->subnets[i].mask = ~(~(uint32_t) 0 >> atoi(delimiter + 1));
        } else {
            log_error("parse subnet file error!");
            return -1;
        }
        *delimiter = 0;
        if (!inet_aton(line, &addr)) {
            log_error("invalid addr %s in %s:%d", line, path, i + 1);
            return -1;
        }
        list->subnets[i].addr = ntohl(addr.s_addr) & list->subnets[i].mask;
        i++;
    }

    qsort(list->subnets, (size_t) list->len, sizeof(subnet_t), cmp_subnet);

    fclose(fp);
    return 0;
}

void subnet_list_free(subnet_list_t *list) {
    if (list->subnets) {
        xfree(list->subnets);
    }
}

bool ip_in_subnet_list(subnet_list_t *list, struct in_addr *addr) {
    int i = 0;
    in_addr_t ip = ntohl(addr->s_addr);

    for (i = 0; i < list->len; ++i) {
        if (ip < list->subnets[i].addr)
            break;
    }
    if (i == 0) {
        return false;
    } else {
        i -= 1;
        if ((ip & list->subnets[i].mask) == list->subnets[i].addr) {
            return true;
        } else {
            return false;
        }
    }
}

static int cmp_subnet(const void *s1, const void *s2) {
    const subnet_t *s_1 = s1;
    const subnet_t *s_2 = s2;

    return s_1->addr - s_2->addr;
}