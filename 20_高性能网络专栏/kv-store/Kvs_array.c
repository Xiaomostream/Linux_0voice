// singleto 单例模式
// 确保一个类只有一个实例，并提供一个全局访问点来访问该实例。

#include "kvstore.h"
kvs_array_t global_array = {0};

//创建，分配内存
int kvs_array_create(kvs_array_t *inst) {
    if(!inst) return -1;

    if(inst->table) {
        printf("tanle has talloc\n");
        return -1;
    }

    inst->table = kvs_malloc(KVS_ARRAY_SIZE * sizeof(kvs_array_item_t));
    if(!inst->table) {
        return -1;
    }
    inst->idx = 0;
    inst->total = 0;
    return 0;
}

//销毁， 释放内存
void kvs_array_destory(kvs_array_t *inst) {
    if(!inst) return;

    if(inst->table) {
        kvs_free(inst->table);
    }
    //不需要释放传入的inst，符合开闭原则
}

char* kvs_array_get(kvs_array_t *inst, char *key) {
    if(inst == NULL || key == NULL) return NULL;

    int i = 0;
    for (i = 0; i < inst->total; i ++) {
        if(inst->table[i].key == NULL) {
            continue;
        }
        if(strcmp(inst->table[i].key, key) == 0) {
            return inst->table[i].value;
        }
    }
    return NULL;
}

/*
 * @return: <0, errro; =0, success; >0, exist
 */
int kvs_array_set(kvs_array_t *inst, char *key, char *value) {
    if(inst == NULL || key == NULL || value == NULL) return -1;
    if(inst->total == KVS_ARRAY_SIZE && inst->idx == KVS_ARRAY_SIZE-1) return -1;

    //如果当前key已经存在
    char *str = kvs_array_get(inst, key);
    if(str) {
        return 1; //存在
    }

    char *kcopy = kvs_malloc(strlen(key)+1);
    if(kcopy == NULL) return -2;
    memset(kcopy, 0, strlen(key)+1);
    strncpy(kcopy, key, strlen(key)+1);

    char *kvalue = kvs_malloc(strlen(value)+1);
    if(kvalue == NULL) return -2;
    memset(kvalue, 0, strlen(value)+1);
    strncpy(kvalue, value, strlen(value)+1);

    int i = 0;
    for (i = 0; i < inst->total; i ++ ) {
        if(inst->table[i].key == NULL) {
            inst->table[i].key = kcopy;
            inst->table[i].value = kvalue;
            inst->total ++;
            return 0;
        }
    }

    // 如果所有都是非空的
    if(i == inst->total && i < KVS_ARRAY_SIZE) {
        inst->table[i].key = kcopy;
        inst->table[i].value = kvalue;
        inst->total ++;
        return 0;
    }
    else {
        printf("Arrive KVS_ARRAY_SIZE, can't allocate\n");
        return -2;
    }
}

/*
 *@ return < 0, error; = 0, success; > 0, not exist;
 */
int kvs_array_del(kvs_array_t *inst, char *key) {
    if(inst == NULL || key == NULL) return -1;

    int i = 0;
    for (i = inst->idx; i < inst->total; i ++) {
        if(inst->table[i].key == NULL) {
            continue;
        }
        if(strcmp(inst->table[i].key, key) == 0) {
            kvs_free(inst->table[i].key);
            inst->table[i].key = NULL;

            kvs_free(inst->table[i].value);
            inst->table[i].value = NULL;

            inst->idx = i;
            return 0;
        }
    }

    return i;
}

/*
 *@ return < 0, error; = 0, success; > 0, not exist;
 */
int kvs_array_mod(kvs_array_t *inst, char *key, char *value) {
    if(inst == NULL || key == NULL || value == NULL) return -1;
    
    int i = 0;
    for (i = 0; i < inst->total; i ++) {
        if(inst->table[i].key == NULL) {
            continue;
        }
        if(strcmp(inst->table[i].key, key) == 0) {
            kvs_free(inst->table[i].value);

            char *kvalue = kvs_malloc(strlen(value)+1);
            if(kvalue == NULL) return -2;
            memset(kvalue, 0, strlen(value)+1);
            strncpy(kvalue, value, strlen(value)+1);

            inst->table[i].value = kvalue;

            return 0;
        }
    }

    return i;
}

/*
 *@ return = 1, exist; = 0, not exist;
 */
int kvs_array_exist(kvs_array_t *inst, char *key) {
    char *str = kvs_array_get(inst, key);
    if(!str) {
        return 1; //存在
    }
    return 0;
}
