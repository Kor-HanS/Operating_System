#ifndef NODE

#define NODE
	#define RUN_QUEUE_SIZE 25
	#define NULL            0

// run queue 구현을 위한 큐 구조체 해당 큐를 통해 프로세스의 링크드 리스트를 구현합니다.
// 각 큐는 프로세스를 가르킬 head와 tail이 있습니다.

typedef struct queue{
	struct proc* head;
	struct proc* tail;
}queue;

void   push_proc_back(queue* q, struct proc* p);     // queue tail에 삽입    
struct proc*  get_priority_proc(queue* q);           // queue 에서 가장 우선순위 높은 node 뽑기(우선순위 값 최소) 
struct proc*  pop_proc(queue* q, struct proc* p);    // queue 에서 프로세스 노드 제거
int    find_proc(queue* q, struct proc* p);   // queue 에서 해당하는 프로세스 노드 찾기
void   print_queue(queue* q);                  // 큐에 있는 모든 노드 출력

void   init_queue(queue* q);                  // queue 초기화
int    get_least_priority_value(queue* q);    // 큐에 노드가 없을 경우 -1, 그 외 큐에서 가장 작은 우선순위 값 반환

#endif
