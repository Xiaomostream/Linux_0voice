// 使用reactor来做kvstore的网络框架
#include "kvstore.h"

#if ENABLE_ARRAY
extern kvs_array_t global_array;
#endif


void *kvs_malloc(size_t size) {
    return malloc(size);
}

void kvs_free(void *ptr) {
    return free(ptr);
}

const char *command[] = {
    "SET", "GET", "DEL", "MOD", "EXIST"
};

enum {
    KVS_CMD_START = 0,
    KVS_CMD_SET = KVS_CMD_START,
    KVS_CMD_GET,
    KVS_CMD_DEL,
    KVS_CMD_MOD,
    KVS_CMD_EXIST,
    KVS_CMD_COUNT
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
    }
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
    return 0;
}

// port, kvs_protocol
int main(int argc, char *argv[]) {

    if(argc != 2) return -1;
    unsigned int port = atoi(argv[1]);

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
    return 0;
}