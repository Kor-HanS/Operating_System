#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// 설계4 관련 시스템 콜 호출 구현 
// sys_ssualloc(), sys_getvp(), sys_getpp()
int 
sys_ssualloc(void)
{
  int vmem_size;  // 할당할 가상페이지 크기 
  
  // 인자 오류
  if(argint(0, &vmem_size) < 0)
    return -1;
	
  // 추가 할당받을 가상 메모리 크기가 양수가 아니거나
  // 페이지 크기의 배수가 아닐 경우 -1을 반환한다.
  if(vmem_size <= 0 || vmem_size % PGSIZE) 
	return -1;

  struct proc *p = myproc();
  uint oldsz = p->sz;
  uint newsz = p->sz + vmem_size; // 새롭게 할당할 가상 주소 공간
	
  // 커널 침범 overflow 여부 확인 및 새로운 크기가 예전 크기보다 작은지 확인
  if(newsz >= KERNBASE || newsz < oldsz)
	  return -1;

  p-> sz = newsz; // 늘어난 가상 주소 공간으로 초기화

  return oldsz;
}

int 
sys_getvp(void)
{
	return get_vpage_count();
}

int 
sys_getpp(void)
{
	return get_ppage_count();
}

