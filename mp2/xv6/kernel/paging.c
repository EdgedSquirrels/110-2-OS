#include "param.h"
#include "types.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "defs.h"
#include "proc.h"

/* NTU OS 2022 */
/* Page fault handler */
int handle_pgfault() {
  /* 1. Find the address that caused the fault */
  /* 2. Allocate a physical memory page for that virtual address */
  /* TODO */
  uint64 va = r_stval();
  struct proc *p = myproc();
  // pte_t *pte = walk(p->pagetable, va, 0);

  if (va >= p->sz || va < p->trapframe->sp) {
    // va > size or va < stackframe
    return -1;
  }

  uint64 pa = PGROUNDDOWN(va);
  char* mem = kalloc();
  if (mem == 0)
    return -1;

  memset(mem, 0, PGSIZE);

  if (mappages(p->pagetable, pa, PGSIZE, (uint64)mem,
      PTE_U | PTE_R | PTE_W | PTE_X) != 0) {
    kfree(mem);
    return -1;
  }
  return 0;

  // panic("not implemented yet\n");
}
