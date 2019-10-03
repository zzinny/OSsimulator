#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include "list_lru_bc.h"
#include "list_lru_nv.h"

int buffer_cache_size, nvram_size;  // 버퍼캐시 사이즈, 비휘발성 메모리 사이즈
int storage_write_count=0;  // 스토리지에 쓰기 연산을 한 횟수
int flush_to_storage=0;     // 스토리지에 주기적 반영한 횟수
int flush_to_nvram=0;       // 비휘발성메모리에 주기적 반영한 횟수

extern list_lru_bc *lru_bc; // 버퍼캐시를 관리하는 LRU list
extern pair_bc *hash_table_bc[HASH_SIZE]; // 버퍼캐시 LRU list의 각 노드에 O(1)에 접근하기 위한 해시테이블
extern list_lru_nv *lru_nv; // 비휘발성메모리를 관리하는 LRU list
extern pair_nv *hash_table_nv[HASH_SIZE]; // 비휘발성메모리 LRU list의 각 노드에 O(1)에 접근하기 위한 해시테이블

/* 
* read_block(int block_num)
* 
* 파일블록의 번호가 block_num인 블록에 대하여 읽기 연산을 한다. 
* 
* 만약 버퍼캐시에 해당 블록이 있다면,
* 버퍼캐시 LRU list에서 해당 블록 위치를 가장 뒤로 옮긴다.
* 
* 만약 버퍼캐시에 해당 블록이 올라와있지 않으면,
* 버퍼캐시 LRU list의 가장 뒤에 해당 블록을 추가한다.
*/
void read_block(int block_num) {
    printf("\n%d번 블록 읽기\n", block_num);

    node_bc *block = list_lru_bc_search(block_num);     // 버퍼캐시 LRU list에서 해당블록 검색

    if(block != NULL) {     // 버퍼캐시에 해당 블록이 있다면
        list_lru_bc_del(block);
        list_lru_bc_add_tail(block);  // 해당 블록을 버퍼캐시 LRU list의 가장 뒤로 옮기기
    }
    else {  // 버퍼캐시에 해당 블록이 올라와있지 않으면
        if(lru_bc->size >= buffer_cache_size) {  // 만약 버퍼 캐시가 꽉 차있으면
            printf("--> LRU에 의해 %d번 블록 버퍼캐시에서 삭제\n", lru_bc->head->next->block_num);
            // 버퍼캐시 LRU list의 가장 앞에 있는 블록이 modified이면 스토리지에 반영
            if(lru_bc->head->next->modified == 1) {
                printf("--> %d번 블록 modified이므로 스토리지에 반영\n", lru_bc->head->next->block_num);
                storage_write_count += 1;   // 총 스토리지 쓰기 횟수 1 증가
            }
            hash_table_bc_del(lru_bc->head->next->block_num);
            node_bc *lru_block = lru_bc->head->next;
            list_lru_bc_del(lru_block);    // 버퍼캐시 LRU list의 가장 앞에 있는 블록 제거
            free(lru_block);
        }

        node_bc *new = node_bc_init(block_num);
        list_lru_bc_add_tail(new);     // 버퍼캐시 LRU list의 가장 뒤에 해당 블록 추가
        hash_table_bc_add(block_num, new);
    }
}

/* 
* write_block(int block_num)
* 
* 파일블록의 번호가 block_num인 블록에 대하여 쓰기 연산을 한다. 
* 
* 블록의 상태를 modified로 만들고, 쓰기 연산 횟수를 1 증가시킨다.
* 또한 버퍼캐시 LRU list에서 해당 블록 위치를 가장 뒤로 옮긴다.
*/
void write_block(int block_num) {
    printf("\n%d번 블록 쓰기\n", block_num);

    node_bc *block = list_lru_bc_search(block_num); // 버퍼캐시 LRU list에서 해당블록 검색

    block->modified = 1;        // 블록 상태를 modified로 바꾸기
    block->write_count += 1;    // 블록의 쓰기연산 횟수를 1증가
    list_lru_bc_del(block);
    list_lru_bc_add_tail(block);   // 해당 블록을 버퍼캐시 LRU list의 가장 뒤로 옮기기
}


/* 
* flush()
* 
* 5초 단위로 버퍼캐시의 modified 블록들을 쓰기 연산 횟수에 따라 스토리지 또는 비휘발성메모리에 반영한다.
* 
* 블록의 쓰기연산 횟수가 1이면 수정된 내용을 스토리지에 반영하고,
* 블록의 쓰기연산 횟수가 1초과이면 비휘발성메모리에 반영한다.
* 수정된 내용이 스토리지 또는 비휘발성메모리에 반영되면,
* 블록의 상태는 unmodified가 되고 쓰기연산 회숫도 0으로 초기화된다.
*/
void* flush() {
    int idx;
    node_bc *cur;
    node_nv *block;

    while(1) {
        cur = lru_bc->head;
        printf("\n주기적 반영 시작\n");
        while(cur->next != lru_bc->head) {  // 리스트의 처음부터 끝까지 순회
            cur = cur->next;
            if(cur->modified == 1) {        // 블록의 상태가 modified이면 스토리지 또는 비휘발성 메모리에 반영
                if(cur->write_count == 1) { // 블록의 쓰기연산 횟수가 1이면 스토리지에 반영
                    printf("--> %d번 블록 스토리지에 반영\n", cur->block_num);
                    storage_write_count += 1;   // 총 스토리지 쓰기 횟수 1 증가
                    flush_to_storage += 1;      // 스토리지로 주기적 반영한 횟수 1 증가
                }
                // 블록의 쓰기연산 횟수가 1초과이면 비휘발성 메모리에 반영
                else if(cur->write_count > 1) {
                    printf("--> %d번 블록 비휘발성메모리에 반영\n", cur->block_num);
                    // 비휘발성메모리 LRU list에서 해당블록 검색
                    block = list_lru_nv_search(cur->block_num);
                    if(block != NULL) { // 비휘발성메모리에 해당 블록이 있는 경우
                        list_lru_nv_del(block);
                        // 해당 블록 위치를 비휘발성메모리 LRU list의 가장 뒤로 옮기기
                        list_lru_nv_add_tail(block);
                    }
                    else {  // 비휘발성메모리에 해당 블록이 없는 경우
                        if(lru_nv->size >= nvram_size) {  // 비휘발성메모리가 꽉 차있으면
                            printf("--> LRU에 의해 %d번 블록 비휘발성메모리에서 삭제 및 스토리지에 반영\n", lru_nv->head->next->block_num);
                            hash_table_nv_del(lru_nv->head->next->block_num);
                            // 비휘발성메모리 LRU list의 가장 앞에 있는 블록 제거
                            node_nv *lru_block = lru_nv->head->next;
                            list_lru_nv_del(lru_block);
                            free(lru_block);
                            storage_write_count += 1;   // 블록의 변경내용을 스토리지에 반영
                        }
                        node_nv *new = node_nv_init(cur->block_num);
                        list_lru_nv_add_tail(new);  // 비휘발성메모리 LRU list의 가장 뒤에 해당 블록 추가
                        hash_table_nv_add(cur->block_num, new);
                    }
                    flush_to_nvram += 1; // 비휘발성메모리로 주기적 반영한 횟수 1 증가
                }
                cur->modified = 0;  // unmodified 상태로 변경
                cur->write_count = 0;   // 블록의 쓰기 연산 횟수 초기화
            }
        }
        printf("주기적 반영 끝\n");
        usleep(5000000);    // 5초 대기
    }
}

int main() {
    char file_path[20] = "./";  // 입력 파일 경로   
    char file_name[20];         // 입력 파일명
    FILE *fp;
    int block_num, operation, t; // 블록 번호, 연산 종류, 요청 시간
    pthread_t p_thread[1];

    list_lru_bc_init();     // 버퍼캐시 LRU list 생성 및 초기화
    list_lru_nv_init();     // 비휘발성메모리 LRU list 생성 및 초기화

    printf("파일명: ");
    scanf("%s", file_name);
    strcat(file_path, file_name);
    fp = fopen(file_path, "r");

    fscanf(fp, "%d%d", &buffer_cache_size, &nvram_size); // 버퍼캐시 및 비휘발성메모리 사이즈 입력받기

    pthread_create(&p_thread[0], NULL, flush, NULL);    // 다른 스레드에서 periodic flush 실행

    while(!feof(fp)) {
        fscanf(fp, "%d%d%d", &block_num, &operation, &t);
        if(operation == 0) {    // 블록 읽기
            read_block(block_num);
        }
        else if(operation == 1){  // 블록 쓰기
            write_block(block_num);
        }
        usleep(1000000);    // 1초 딜레이
    }

    fclose(fp);
    pthread_cancel(p_thread[0]);

    // 결과 출력
    printf("----------------결과 출력----------------\n");
    printf("총 스토리지 쓰기 횟수: %d\n", storage_write_count);
    printf("스토리지로 주기적 반영한 횟수: %d\n", flush_to_storage);
    printf("비휘발성메모리로 주기적 반영한 횟수: %d\n", flush_to_nvram);

    printf("----------------버퍼캐시 LRU list----------------\n");
    printf("head->");
    node_bc *cur1 = lru_bc->head;
    while(cur1->next != lru_bc->head) {
        cur1 = cur1->next;
        printf("%d->", cur1->block_num);
    }
    printf("tail\n");

    printf("----------------비휘발성메모리 LRU list----------------\n");
    printf("head->");
    node_nv *cur2 = lru_nv->head;
    while(cur2->next != lru_nv->head) {
        cur2 = cur2->next;
        printf("%d->", cur2->block_num);
    }
    printf("tail\n");
}