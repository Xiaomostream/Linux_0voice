// 使用reactor来做kvstore的网络框架
#include "kvstore.h"

#if ENABLE_ARRAY
extern kvs_array_t global_array;
#endif

#if ENABLE_ARRAY
extern kvs_rbtree_t global_rbtree;
#endif

#if ENABLE_HASH
extern kvs_hash_t global_hash;
#endif

void *kvs_malloc(size_t size) {
    return malloc(size);
}

void kvs_free(void *ptr) {
    return free(ptr);
}

const char *command[] = {
    "SET", "GET", "DEL", "MOD", "EXIST",
    "RSET", "RGET", "RDEL", "RMOD", "REXIST",
    "HSET", "HGET", "HDEL", "HMOD", "HEXIST",
};

enum {
    KVS_CMD_START = 0,
    // array
    KVS_CMD_SET = KVS_CMD_START,
    KVS_CMD_GET,
    KVS_CMD_DEL,
    KVS_CMD_MOD,
    KVS_CMD_EXIST,
    // rbtree
    KVS_CMD_RSET,
    KVS_CMD_RGET,
    KVS_CMD_RDEL,
    KVS_CMD_RMOD,
    KVS_CMD_REXIST,
    // hash
    KVS_CMD_HSET,
    KVS_CMD_HGET,
    KVS_CMD_HDEL,
    KVS_CMD_HMOD,
    KVS_CMD_HEXIST,
    KVS_CMD_COUNT,
};
const char *response[] = {

};

int kvs_split_token(char *msg, char *tokens[]) {
    if(msg == NULL || tokens == NULL) return -1;
    char *token = strtok(msg, " ");
    //strtok：第一个参数为要分割的字符串。第一次调用时传入待分割的字符串，后续调用时传入 NULL 以继续分割。
    int idx = 0;
    while(token != NULL) {
        tokens[idx ++] = token;
        //printf("tokens[%d]: %s\n", idx-1, token);
        token = strtok(NULL, " "); 
    }
    return idx;
}

// SET Key Value
// tokens[0] : SET
// tokens[1] : Key
// tokens[2] : Value
int kvs_fliter_protocol(char **tokens, int count, char *response) {
    if(tokens == NULL || count == 0 || response == NULL) return -1;


    int cmd;
    for (cmd = KVS_CMD_START; cmd < KVS_CMD_COUNT; cmd ++) {
        if(strcmp(tokens[0], command[cmd]) == 0) {
            break;
        }
    }

    int length = 0;
    int ret = 0;
    char *key = tokens[1];
    char *value = tokens[2];

    switch(cmd) {
#if ENABLE_ARRAY
        // array
        case KVS_CMD_SET:
            ret = kvs_array_set(&global_array, key, value);
            if(ret < 0) {
                length = sprintf(response, "ERROR\r\n");
            } else if(ret == 0) {
                length = sprintf(response, "OK\r\n");
            } else {
                length = sprintf(response, "EXIST\r\n");
            }
            break;
        case KVS_CMD_GET:
            char *result = kvs_array_get(&global_array, key);
            if(result == NULL) {
                length = sprintf(response, "NOT EXIST\r\n");
            } else {
                length = sprintf(response, "%s\r\n", result);
            }
            //printf("## result: %s\n", result);
            break;
        case KVS_CMD_DEL:
            ret = kvs_array_del(&global_array, key);
            if(ret < 0) {
                length = sprintf(response, "ERROR\r\n");
            } else if(ret == 0) {
                length = sprintf(response, "OK\r\n");
            } else {
                length = sprintf(response, "NOT EXIST\r\n");
            }
            break;
        case KVS_CMD_MOD:
            ret = kvs_array_mod(&global_array, key, value);
            if(ret < 0) {
                length = sprintf(response, "ERROR\r\n");
            } else if(ret == 0) {
                length = sprintf(response, "OK\r\n");
            } else {
                length = sprintf(response, "NOT EXIST\r\n");
            }
            break;
        case KVS_CMD_EXIST:
            ret = kvs_array_exist(&global_array, key);
            if(ret == 0) {
                length = sprintf(response, "EXIST\r\n");
            } else {
                length = sprintf(response, "NOT EXIST\r\n");
            }
            break;
#endif

#if ENABLE_RBTREE
        // rbtree
        case KVS_CMD_RSET:
            ret = kvs_rbtree_set(&global_rbtree, key, value);
            if(ret < 0) {
                length = sprintf(response, "ERROR\r\n");
            } else if(ret == 0) {
                length = sprintf(response, "OK\r\n");
            } else {
                length = sprintf(response, "EXIST\r\n");
            }
            break;
        case KVS_CMD_RGET:
            char *rb_result = kvs_rbtree_get(&global_rbtree, key);
            if(rb_result == NULL) {
                length = sprintf(response, "NOT EXIST\r\n");
            } else {
                length = sprintf(response, "%s\r\n", rb_result);
            }
            //printf("## result: %s\n", result);
            break;
        case KVS_CMD_RDEL:
            ret = kvs_rbtree_del(&global_rbtree, key);
            if(ret < 0) {
                length = sprintf(response, "ERROR\r\n");
            } else if(ret == 0) {
                length = sprintf(response, "OK\r\n");
            } else {
                length = sprintf(response, "NOT EXIST\r\n");
            }
            break;
        case KVS_CMD_RMOD:
            ret = kvs_rbtree_mod(&global_rbtree, key, value);
            if(ret < 0) {
                length = sprintf(response, "ERROR\r\n");
            } else if(ret == 0) {
                length = sprintf(response, "OK\r\n");
            } else {
                length = sprintf(response, "NOT EXIST\r\n");
            }
            break;
        case KVS_CMD_REXIST:
            ret = kvs_rbtree_exist(&global_rbtree, key);
            if(ret == 0) {
                length = sprintf(response, "EXIST\r\n");
            } else {
                length = sprintf(response, "NOT EXIST\r\n");
            }
            break;
#endif

#if ENABLE_HASH
        // hash
        case KVS_CMD_HSET:
            ret = kvs_hash_set(&global_hash, key, value);
            if(ret < 0) {
                length = sprintf(response, "ERROR\r\n");
            } else if(ret == 0) {
                length = sprintf(response, "OK\r\n");
            } else {
                length = sprintf(response, "EXIST\r\n");
            }
            break;
        case KVS_CMD_HGET:
            char *hash_result = kvs_hash_get(&global_hash, key);
            if(hash_result == NULL) {
                length = sprintf(response, "NOT EXIST\r\n");
            } else {
                length = sprintf(response, "%s\r\n", hash_result);
            }
            //printf("## result: %s\n", result);
            break;
        case KVS_CMD_HDEL:
            ret = kvs_hash_del(&global_hash, key);
            if(ret < 0) {
                length = sprintf(response, "ERROR\r\n");
            } else if(ret == 0) {
                length = sprintf(response, "OK\r\n");
            } else {
                length = sprintf(response, "NOT EXIST\r\n");
            }
            break;
        case KVS_CMD_HMOD:
            ret = kvs_hash_mod(&global_hash, key, value);
            if(ret < 0) {
                length = sprintf(response, "ERROR\r\n");
            } else if(ret == 0) {
                length = sprintf(response, "OK\r\n");
            } else {
                length = sprintf(response, "NOT EXIST\r\n");
            }
            break;
        case KVS_CMD_HEXIST:
            ret = kvs_hash_exist(&global_hash, key);
            if(ret == 0) {
                length = sprintf(response, "EXIST\r\n");
            } else {
                length = sprintf(response, "NOT EXIST\r\n");
            }
            break;
#endif
        default:
            assert(0);
    }
    //printf("## response:%s\n", response);
    return length;
}
/*
 *msg: request message
 *length: length of request message
 *response: need to send
 *@return:  length
*/
int kvs_protocol(char *msg, int length, char *response) { 

// SET Key Value
// GET Key
// DEL Key
    if(msg == NULL || length <= 0 || response == NULL) return -1;
    //printf("recv %d : %s\n", length, msg);

    char *tokens[KVS_MAX_TOKENS] = {0};

    int count = kvs_split_token(msg, tokens);
    if(count == -1) return -1;
    // memcpy(response, msg, length);
    return kvs_fliter_protocol(tokens, count, response);
}

int init_kvengine(void) {

#if ENABLE_ARRAY
    memset(&global_array, 0, sizeof(kvs_array_t));
    kvs_array_create(&global_array);
#endif

#if ENABLE_RBTREE
    memset(&global_rbtree, 0, sizeof(kvs_rbtree_t));
    kvs_rbtree_create(&global_rbtree);
#endif

#if ENABLE_HASH
    memset(&global_hash, 0, sizeof(kvs_hash_t));
    kvs_hash_create(&global_hash);
#endif
    return 0;
}

void dest_kvengine(void) {
#if ENABLE_ARRAY
    kvs_array_destory(&global_array);
#endif

#if ENABLE_RBTREE
    kvs_rbtree_destory(&global_rbtree);
#endif

#if ENABLE_HASH
    kvs_hash_destory(&global_hash);
#endif
}
// port, kvs_protocol
int main(int argc, char *argv[]) {

    if(argc != 2) {
        printf("Params error!\n");
        return -1;
    }
    unsigned short port = atoi(argv[1]);

    init_kvengine();

    //printf("%d %d\n", NETWORK_SELECT, NETWORK_NTYCO);
#if (NETWORK_SELECT == NETWORK_REACTOR)
    reactor_start(port, kvs_protocol);
#elif (NETWORK_SELECT == NETWORK_NTYCO)
    Ntyco_start(port, kvs_protocol);
#elif (NETWORK_SELECT == NETWORK_PROACTOR)
    proactor_start(port, kvs_protocol);
#endif
    // printf("Hello!\n");
    dest_kvengine();
    return 0;
}