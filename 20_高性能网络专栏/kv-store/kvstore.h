
#ifndef __KV_STORE_H__
#define __KV_STORE_H__


typedef int (*msg_handler)(char *msg, int length, char *response);

const char *command[] = {
    "SET", "GET", "DEL", "MOD", "EXIST"
};
const char *response[] = {

};

#endif