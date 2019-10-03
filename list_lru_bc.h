#ifndef HASH_SIZE
#define HASH_SIZE 100   // 해시테이블 사이즈
#endif


/* 
* struct node_bc
* 
* 버퍼 캐시를 관리하는 LRU list의 노드
*/
typedef struct node_bc {
    int block_num;      // 블록 번호
    int write_count;    // 1회성 쓰기인지 확인하기 위함
    int modified;       // 0: unmodified, 1:modified
    struct node_bc* prev;   // 이전 노드를 가리킴
    struct node_bc* next;   // 다음 노드를 가리킴
} node_bc;


/* 
* struct list_lru_bc
* 
* 버퍼 캐시를 관리하는 LRU list(Circular doubly linked list로 구현)
*/
typedef struct list_lru_bc {
    int size;       // 리스트의 사이즈
    node_bc *head;  // 리스트의 헤드
} list_lru_bc;


/* 
* struct pair_bc
* 
* 버퍼캐시 LRU list의 각 노드에 O(1)에 접근하기 위한 해시테이블의 각 요소
*/
typedef struct pair_bc {
    int key;        // 파일블록 번호
    node_bc *value; // 블록에 해당하는 버퍼캐시 LRU list 노드의 주소
} pair_bc;


node_bc* node_bc_init(int block_num);   // node_bc 생성 및 초기화

pair_bc* pair_bc_init(int key, node_bc *value); // pair_bc 생성 및 초기화

void list_lru_bc_init();    // 버퍼캐시 LRU list 생성 및 초기화

void list_lru_bc_del(node_bc *entry);   // 버퍼캐시 LRU list에서 특정 노드 삭제

void list_lru_bc_add_tail(node_bc *entry);  // 버퍼캐시 LRU list의 가장 끝에 노드 추가

node_bc *list_lru_bc_search(int block_num); // 버퍼캐시 LRU list에서 블록 번호가 block_num인 노드 검색

void hash_table_bc_add(int block_num, node_bc *_new);   // 해시테이블에 요소 추가

void hash_table_bc_del(int block_num);  // 해시테이블에서 요소 삭제
