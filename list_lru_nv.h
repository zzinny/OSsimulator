#ifndef HASH_SIZE
#define HASH_SIZE 100   // 해시테이블 사이즈
#endif

/* 
* struct node_nv
* 
* 비휘발성메모리를 관리하는 LRU list의 노드
*/
typedef struct node_nv {
    int block_num;  // 블록 번호
    struct node_nv* prev;   // 이전 노드를 가리킴
    struct node_nv* next;   // 다음 노드를 가리킴
} node_nv;


/* 
* struct list_lru_nv
* 
* 비휘발성메모리를 관리하는 LRU list(Circular doubly linked list로 구현)
*/
typedef struct list_lru_nv {
    int size;       // 리스트 사이즈
    node_nv *head;  // 리스트 헤드
} list_lru_nv;


/* 
* struct pair_nv
* 
* 비휘발성메모리 LRU list의 각 노드에 O(1)에 접근하기 위한 해시테이블의 각 요소
*/
typedef struct pair_nv {
    int key;        // 블록 번호
    node_nv *value; // 블록에 해당하는 비휘발성메모리 LRU list 노드의 주소
} pair_nv;

node_nv* node_nv_init(int block_num);   // node_nv 생성 및 초기화

pair_nv* pair_nv_init(int key, node_nv *value); // pair_nv 생성 및 초기화

void list_lru_nv_init();    // 비휘발성메모리 LRU list 생성 및 초기화

void list_lru_nv_del(node_nv *entry);   // 비휘발성메모리 LRU list에서 노드 삭제

void list_lru_nv_add_tail(node_nv *entry);      // 비휘발성메모리 LRU list의 가장 끝에 노드 추가

node_nv *list_lru_nv_search(int block_num);     // 비휘발성메모리 LRU list에서 블록 번호가 block_num인 노드 검색

void hash_table_nv_add(int block_num, node_nv *_new);   // 해시테이블에 노드 추가

void hash_table_nv_del(int block_num);  // 해시테이블에서 노드 삭제