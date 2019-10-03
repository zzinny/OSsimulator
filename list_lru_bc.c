#include <stdlib.h>
#include "list_lru_bc.h"

list_lru_bc *lru_bc;
pair_bc *hash_table_bc[100];

node_bc* node_bc_init(int block_num) {
    node_bc *_new = (node_bc*)malloc(sizeof(node_bc));
    _new->block_num = block_num;
    _new->write_count = 0;
    _new->modified = 0;
    _new->prev = NULL;
    _new->next = NULL;

    return _new;
}

pair_bc* pair_bc_init(int key, node_bc *value) {
    pair_bc* _new = (pair_bc*)malloc(sizeof(pair_bc));
    _new->key = key;
    _new->value = value;

    return _new;
}

void list_lru_bc_init() {
    lru_bc = (list_lru_bc*)malloc(sizeof(list_lru_bc));
    lru_bc->size = 0;
    lru_bc->head = node_bc_init(-1);
    lru_bc->head->prev = lru_bc->head;
    lru_bc->head->next = lru_bc->head;
}

void list_lru_bc_del(node_bc *entry) {
    entry->prev->next = entry->next;
    entry->next->prev = entry->prev;

    lru_bc->size -= 1;
}

void list_lru_bc_add_tail(node_bc *entry) {
    entry->prev = lru_bc->head->prev;
    entry->next = lru_bc->head;
    lru_bc->head->prev->next = entry;
    lru_bc->head->prev = entry;

    lru_bc->size += 1;
}

node_bc *list_lru_bc_search(int block_num) {
    int idx = block_num % HASH_SIZE;

    while(hash_table_bc[idx] != NULL) {
        if(hash_table_bc[idx]->value->block_num == block_num) { // found
            return hash_table_bc[idx]->value;
        }
        idx = (idx + 1) % HASH_SIZE;
    }

    return NULL;    // serach fail
}

void hash_table_bc_add(int block_num, node_bc *_new) {
    int idx = block_num % HASH_SIZE;

    while(hash_table_bc[idx] != NULL) {
        idx = (idx + 1) % HASH_SIZE;
    }

    hash_table_bc[idx] = pair_bc_init(block_num, _new);
}

void hash_table_bc_del(int block_num) {
    int idx = block_num % HASH_SIZE;

    while(hash_table_bc[idx] != NULL) {
        if(hash_table_bc[idx]->value->block_num == block_num) {
            hash_table_bc[idx] = NULL;
            break;
        }
        idx = (idx + 1) % HASH_SIZE;
    }
}
