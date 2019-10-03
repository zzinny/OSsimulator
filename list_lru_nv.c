#include <stdlib.h>
#include "list_lru_nv.h"

list_lru_nv *lru_nv;
pair_nv *hash_table_nv[100];

node_nv* node_nv_init(int block_num) {
    node_nv *_new = (node_nv*)malloc(sizeof(node_nv));
    _new->block_num = block_num;
    _new->prev = NULL;
    _new->next = NULL;

    return _new;
}

pair_nv* pair_nv_init(int key, node_nv *value) {
    pair_nv* _new = (pair_nv*)malloc(sizeof(pair_nv));
    _new->key = key;
    _new->value = value;

    return _new;
}

void list_lru_nv_init() {
    lru_nv = (list_lru_nv*)malloc(sizeof(list_lru_nv));
    lru_nv->size = 0;
    lru_nv->head = node_nv_init(-1);
    lru_nv->head->prev = lru_nv->head;
    lru_nv->head->next = lru_nv->head;
}

void list_lru_nv_del(node_nv *entry) {
    entry->prev->next = entry->next;
    entry->next->prev = entry->prev;

    lru_nv->size -= 1;
}

void list_lru_nv_add_tail(node_nv *entry) {
    entry->prev = lru_nv->head->prev;
    entry->next = lru_nv->head;
    lru_nv->head->prev->next = entry;
    lru_nv->head->prev = entry;

    lru_nv->size += 1;
}

node_nv *list_lru_nv_search(int block_num) {
    int idx = block_num % HASH_SIZE;

    while(hash_table_nv[idx] != NULL) {
        if(hash_table_nv[idx]->value->block_num == block_num) { // found
            return hash_table_nv[idx]->value;
        }
        idx = (idx + 1) % HASH_SIZE;
    }

    return NULL;    // serach fail
}

void hash_table_nv_add(int block_num, node_nv *_new) {
    int idx = block_num % HASH_SIZE;

    while(hash_table_nv[idx] != NULL) {
        idx = (idx + 1) % HASH_SIZE;
    }

    hash_table_nv[idx] = pair_nv_init(block_num, _new);
}

void hash_table_nv_del(int block_num) {
    int idx = block_num % HASH_SIZE;

    while(hash_table_nv[idx] != NULL) {
        if(hash_table_nv[idx]->value->block_num == block_num) {
            hash_table_nv[idx] = NULL;
            break;
        }
        idx = (idx + 1) % HASH_SIZE;
    }
}
