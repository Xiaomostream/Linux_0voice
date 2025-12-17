#include "server.h"
#include <stdio.h>

int ws_request(struct conn *c){
    printf("request: %s\n", c->rbuffer);
}

int ws_response(struct conn *c) {

}