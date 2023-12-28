#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "node.h"


struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int ptable_index; // ptable에 해당하는 인덱스의 노드가 RUNNABLE이라면 런큐에 넣는다.
queue runqueues[RUN_QUEUE_SIZE]; // 링크드리스트를 관리하는 큐의 배열 

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  int index = 0;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
	{
	  ptable_index = index; // 해당 인덱스의 프로세스가 RUNNABLE되면, node로 초기화후, 런큐 삽입
      goto found;
	}

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  if(p->pid == 1 ||p->pid == 2)
	  p->priority = 99;
  else
  {
	  int i,queue_min = 0;
	  int find = 0;
	  
	  for(i = 0; i < RUN_QUEUE_SIZE; i++)
	  {
		  if((queue_min = get_least_priority_value(&runqueues[i])) < 0)
			continue;
		  else
		  {
			  p->priority = queue_min; 
			  find = 1;
			  // 현재 큐에서 우선순위 값을 얻었다면 다음 
			  // 다음 큐보단 값이 작을 것이 자명하기 때문에 break
			 break;
		  }
	  }

	  if(!find)
		  p->priority = 0; // PID 1 , 2 외에 값을 찾지 못함 기본값인 0 설정
  }
  
  p->proc_tick = 0;
  p->cpu_used  = 0;
  p->is_time_set = 0;
  p->remaining_ticks =0;
  p->priority_tick = 0;
  p->next = NULL;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];
  
  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);
  
  int i;
  for(i = 0; i < RUN_QUEUE_SIZE; i++)
  {
	// runqueues의 모든 큐 초기화  
	init_queue(&runqueues[i]);
  }

  p->state = RUNNABLE;
  
  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;
  
  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  cprintf("PID : %d, priority : %d, proc_tick : %d ticks, total_cpu_usage : %d ticks (3)\n",curproc->pid, curproc->priority,curproc->proc_tick, curproc->cpu_used);
  cprintf("PID : %d terminated\n",curproc->pid);
	
  pop_proc(&runqueues[curproc->priority/4],curproc);

  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;

      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;)
  {
	sti();
	acquire(&ptable.lock);
	
  // Makefile SCHED_POLICY 를 DEFAULT / SSU_SCHEDULER 로 선택 하느냐에 따라 스케쥴러 선택 가능
  # ifdef DEFAULT
	struct proc *p;
	for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
	{
		if(p->state != RUNNABLE)
			continue;

		c-> proc = p;
		switchuvm(p);
		p->state = RUNNING;
		#ifdef DEBUG
		if(p)
		{
			cprintf("PID : %d, priority : %d, proc_tick : %d ticks, total_cpu_used : %d ticks (2)\n",p->pid, p->priority, p->proc_tick, p->cpu_used);			
			cprintf("PID : %d, NAME : XV6 SCHEDULER.\n",p->pid);
		}
		#endif

		swtch(&(c->scheduler), p->context);
		switchkvm();

		c->proc = 0;
	}

#elif SSU_SCHEDULER
  struct proc* p;
  int index = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++, index++)
	{
      if(p->state != RUNNABLE)
        continue;
	
	  ptable_index = index;
	  // RUNNABLE인 프로세스가 런큐에 없다면 삽입한다.
	  if(find_proc(&runqueues[p->priority/4],p) < 0)
			push_proc_back(&runqueues[p->priority/4], p);
	}
	
	int i;
	for(i = 0; i < RUN_QUEUE_SIZE; i++)
	{
		// runqueue 에서 가장 우선순위 값이 작은(우선순위가 높은) 프로세스 선택한다.
		if((p = get_priority_proc(&runqueues[i])) == NULL)
			continue;
		// Switch to chosen process.  It is the process's job
		// to release ptable.lock and then reacquire it
		// before jumping back to us.
		c->proc = p;
		switchuvm(p);
		p->state = RUNNING;
		#ifdef DEBUG
			if(p) 
			{
				cprintf("PID : %d, priority : %d, proc_tick : %d ticks, total_cpu_used : %d ticks (2)\n",p->pid, p->priority, p->proc_tick, p->cpu_used);
				cprintf("PID : %d, NAME : SSU_SCHEDULER.\n",p->pid);
			}
		#endif
      
		swtch(&(c->scheduler), p->context);
		switchkvm();
	
		// Process is done running for now.
		// It should have changed its p->state before coming back.
		c->proc = 0;
		break;
	}
#else
#endif

	release(&ptable.lock);
  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  
  intena = mycpu()->intena; // 인터럽트 가능 여부를 intena 변수에 저장한다.
  swtch(&p->context, mycpu()->scheduler); // swtch() : 스케쥴러와 문맥교환 담당
  mycpu()->intena = intena; // 인터럽트 가능 여부 복구
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;
  
  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {

    if(p->state == SLEEPING && p->chan == chan)
	{
      p->state = RUNNABLE;
	 
	  if(p->pid != 1 && p->pid != 2) // pid 1 2를 제외한 프로세스 wake up 시 우선순위 0
		p->priority = 0;
	}
  }
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

int set_sche_info(int priority, int remaining_tick)
{
	if(priority < 0 || remaining_tick < 0)
		return -1;

	struct proc* p = myproc();

	cprintf("set_sche_info() pid = %d\n", p->pid);

	acquire(&ptable.lock);  //DOC: yieldlock
	// 프로세스 초기 priority 및 종료 타이머 인자 전달
	// 이전 런큐의 우선순위 배열에서 뽑아 새로운 인덱스의 큐로 옮겨주기
	pop_proc(&runqueues[p->priority/4], p);
	
	p->priority = priority;
	p->is_time_set = 1;
	p->remaining_ticks = remaining_tick;
	p->priority_tick = 0;

	push_proc_back(&runqueues[p->priority/4], p);

	release(&ptable.lock);

	return 0;
}

void calculate_new_priority()
{
	struct proc * p;
	// 60tick 마다 trap.c에서 호출 
    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
	{
      if(p->state == RUNNABLE || p->state == RUNNING || p->state == SLEEPING)
	  {
		// IDLE 프로세스는 제외(PID 1,2) == 우선순위 99 고정 
		// 예시에서 확인해본바로, SLEEPING -> RUNNABLE 에도 우선순위 99 고정
		if(p->pid == 1 || p->pid == 2)
			continue;
	
		// 우선 순위 재조정 및 변수 초기화 
		pop_proc(&runqueues[p->priority/4],p);

		int new_priority = p->priority + (p->priority_tick / 10);
		
		// 프로세스가 가질 수 있는  우선순위는 0 ~ 99 입니다.
		if(new_priority > 99)
			new_priority = 99; 
		
		p->priority = new_priority;


		push_proc_back(&runqueues[p->priority/4],p);
		
		// 우선순위 재조정 이후 관련 변수 초기화 
		p->priority_tick = 0;
	  }
	}

	release(&ptable.lock);
}


