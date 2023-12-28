#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "node.h"

void push_proc_back(queue* q, struct proc* p)
{
	if(q->head == NULL)
	{
		q->head = p;
		q->tail = p;
	}
	else
	{
		q->tail->next = p;
		q->tail = p;
	}
}

struct proc* get_priority_proc(queue* q)
{
	if(q->head == NULL)
		return NULL;
	
	struct proc* now_proc = q->head;
	struct proc* most_priority = NULL;

	// O(큐에 있는 노드 수)
	while(now_proc != NULL)
	{
		if(now_proc->state != RUNNABLE)
		{
			now_proc = now_proc->next;
			continue;
		}

		// 우선순위는 값이 낮을 수록 우선순위가 높다.
		if(most_priority == NULL || now_proc->priority < most_priority->priority)
			most_priority = now_proc;

		now_proc = now_proc->next;
	}

	return most_priority;
}

struct proc* pop_proc(queue* q, struct proc* p)
{
	struct proc* now_proc = q->head;
	struct proc* prev_proc = NULL;

	while(now_proc != NULL)
	{
		// 해당하는 프로세스 찾았다.
		if(now_proc->pid == p->pid)
		{
			// 첫번째 프로세스 였다
			if(prev_proc == NULL)
			{
				q->head = now_proc->next;

				// 마지막 프로세스 였다.
				if(now_proc->next == NULL)
				   q->tail = NULL;
			}
			else 
			{
				prev_proc->next = now_proc->next;

				// 마지막 프로세스 였다.
				if(now_proc->next == NULL)
					q->tail = prev_proc;
			}

			now_proc->next = NULL;

			return now_proc;
		}

		prev_proc = now_proc;
		now_proc = now_proc->next;
	}

	return NULL;
}

int find_proc(queue* q, struct proc* p)
{
   struct proc* now_proc = q->head;

	while(now_proc != NULL)
	{
		if(now_proc->pid == p->pid)
			return 1;
		
		now_proc = now_proc->next;
	}

	return -1;
}

void print_queue(queue* q)
{
	int count = 1;
	struct proc* now_proc = q->head;

	while(now_proc != NULL)
	{
		cprintf("NODE# : %d, PID : %d, priority : %d\n",count,now_proc->pid, now_proc->priority);
		count++;
		now_proc = now_proc->next;
	}
}

void init_queue(queue* q)
{
	q->head = NULL;
	q->tail = NULL;
}

int get_least_priority_value(queue* q)
{
	const int INF = 987654321;
	int least_priority_value = INF;

	// 큐에 노드가 없다.
	if(q->head == NULL)
		return -1;

	struct proc* now_proc = q->head;

	while(now_proc != NULL)
	{
		if(now_proc->pid == 1 || now_proc->pid == 2)
		{
			now_proc = now_proc->next;
			continue; // PID 1 ,2 무시
		}

		if(now_proc->priority < least_priority_value)
			least_priority_value = now_proc->priority;

		now_proc = now_proc->next;
	}

	if(least_priority_value == INF)
		return -1;
	else 
		return least_priority_value;
}
