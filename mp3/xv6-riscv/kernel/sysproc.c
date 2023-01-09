#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"



uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
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

uint64
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

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}


// for mp3
uint64
sys_thrdstop(void)
{
  int delay, thrdstop_context_id;
  uint64 handler, handler_arg;
  if (argint(0, &delay) < 0)
    return -1;
  if (argint(1, &thrdstop_context_id) < 0)
    return -1;
  if (argaddr(2, &handler) < 0)
    return -1;
  if (argaddr(3, &handler_arg) < 0)
    return -1;

  // void (*thrdstop_handler)(void*) = (void*)handler;
  // void *thrdstop_arg = (void*)handler_arg;
  // You shoud know the calling convention of the risc-v
  
  // printf("thrdstop: delay:%d thrdstop_context_id:%d handler:%d handlearg:%d\n", 
  //   delay, thrdstop_context_id, handler, handler_arg);
  // printf("thrdstop: uptime: %d\n", sys_uptime());

  struct proc *p = myproc();

  if (thrdstop_context_id == -1) {
    // assign a new thrdstop_contex_id
    for (thrdstop_context_id = 0; thrdstop_context_id < MAX_THRD_NUM; thrdstop_context_id++) {
      if (!p->thrdstop_isValid[thrdstop_context_id]) {
        p->thrdstop_isValid[thrdstop_context_id] = 1;
        break;
      }
    }
  }

  // update thrdstop info
  p->thrdstop_ActiveID = thrdstop_context_id;
  p->thrdstop_ticked = 0;
  p->thrdstop_handler = (void*) handler;
  p->thrdstop_arg = (void*) handler_arg;
  p->thrdstop_delay = delay;

  // for restoration, it should be the last step
  p->handle_framesave = thrdstop_context_id;
  return thrdstop_context_id;
}

// for mp3
uint64
sys_cancelthrdstop(void)
{
  int thrdstop_context_id, is_exit;
  if (argint(0, &thrdstop_context_id) < 0)
    return -1;
  if (argint(1, &is_exit) < 0)
    return -1;

  struct proc *p = myproc();
  
  if (is_exit == 0) {
    if (thrdstop_context_id != -1) {
      // store the current context
      p->handle_framesave = thrdstop_context_id;
    }

  }
  else {
    if (thrdstop_context_id != -1)
      p->thrdstop_isValid[thrdstop_context_id] = 0;
  }
  p->thrdstop_ActiveID = -1;

  // return the time that the process ticks
  return p->thrdstop_ticked;
}

// for mp3
uint64
sys_thrdresume(void)
{
  int thrdstop_context_id;
  if (argint(0, &thrdstop_context_id) < 0)
    return -1;
  
  // debug
  
  struct proc* p = myproc();

  p->handle_frameload = thrdstop_context_id;

  return 0;
}
