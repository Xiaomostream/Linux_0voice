
#ifndef __KV_STORE_H__
#define __KV_STORE_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#define NETWORK_REACTOR 0
#define NETWORK_NTYCO   1
#define NETWORK_PROACTOR 2

#define NETWORK_SELECT  PROACTOR

#define KVS_MAX_TOKENS  128

#define ENABLE_ARRAY    1

typedef int (*msg_handler)(char *msg, int length, char *response);

extern int reactor_start(unsigned int port, msg_handler handler);
extern int Ntyco_start(unsigned int port, msg_handler handler);
extern int proactor_start(unsigned int port, msg_handler handler);


#if ENABLE_ARRAY

typedef struct kvs_array_item_s {
    char *key;
    char *value;
} kvs_array_item_t;

#define KVS_ARRAY_SIZE  1024

typedef struct kvs_array_s {
    kvs_array_item_t *table;
    int idx;
    int total;
} kvs_array_t;

int kvs_array_create(kvs_array_t *inst);
void kvs_array_destory(kvs_array_t *inst);
char* kvs_array_get(kvs_array_t *inst, char *key);
int kvs_array_set(kvs_array_t *inst, char *key, char *value);
int kvs_array_del(kvs_array_t *inst, char *key);
int kvs_array_mod(kvs_array_t *inst, char *key, char *value);
int kvs_array_exist(kvs_array_t *inst, char *key);
#endif

void *kvs_malloc(size_t size);

void kvs_free(void *ptr);

#endif

//gcc -o kvstore kvstore.c reactor.c proactor.c Ntyco.c -I ./NtyCo/core/ -L ./NtyCo/ -luring -lntyco