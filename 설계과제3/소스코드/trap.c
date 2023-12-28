#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"

// 기존 XV6 스케쥴러와 SSU_SCHEDULER를 선택함에 따라 TIME QUANTUM을 다르게 설정
#ifdef DEFAULT 
 #define TIMEQUANTUM 1
#elif SSU_SCHEDULER
 #define TIMEQUANTUM  30  // 기존 1 tick의 타임 슬라이싱을 30으로 맞추기 위한 매크로
#endif

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;
uint cpu_used_ticks;

void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
  if(tf->trapno == T_SYSCALL){
    if(myproc()->killed)
      exit();
    myproc()->tf = tf;
    syscall();
    if(myproc()->killed)
      exit();
    return;
  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;

  //PAGEBREAK: 13
  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();

  if(myproc() && myproc()->state == RUNNING && tf->trapno == T_IRQ0 + IRQ_TIMER && (tf->cs&3) == DPL_USER)
  {
	cpu_used_ticks++;

	// 틱 관련 변수들 증가
	myproc()->proc_tick++;
	myproc()->cpu_used++;
	myproc()->priority_tick++;

	// 60 tick 마다 priority 재 계산
	if((cpu_used_ticks % 60) == 0)
		calculate_new_priority();
	
	// set_sche_info() 통해 만든 지정한 시간 지나면 자식 프로세스들 종료 
	if(myproc()->is_time_set && myproc()->remaining_ticks)
	{
		myproc()->remaining_ticks--;
		if(myproc()->remaining_ticks == 0)
			myproc()->killed = 1;
	}
  }
		
  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if(myproc() && myproc()->state == RUNNING && tf->trapno == T_IRQ0 + IRQ_TIMER)
	// 타임 슬라이싱 (TIMEQUANTUM == 30)
	if(myproc()->proc_tick == TIMEQUANTUM)
	{
		cprintf("PID : %d, priority : %d, proc_tick : %d ticks, total_cpu_usage : %d ticks (1)\n",myproc()->pid,myproc()->priority,myproc()->proc_tick,myproc()->cpu_used);
		myproc()->proc_tick = 0;
		yield();
	}

  // Check if the process has been killed since we yielded
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();
}
