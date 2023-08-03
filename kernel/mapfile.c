#include "types.h"
#include "param.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"
#include "fcntl.h"

#define READ  13
#define WRITE 15

int
getmd(uint64 addr)
{
  struct proc *p = myproc();
  uint64 va;

  for (int md = 0; md < NOMAPS; md++) {
    va = p->mfile[md].va;
    if(!va)
      continue;
    if(va <= addr && addr < va + PGROUNDUP(p->mfile[md].size))
      return md;
  }
  return -1;
}

int
mfilealloc(struct proc *p, int fd)
{
  int md;
  for(md = 0; md < NOMAPS; md++)
    if(!p->mfile[md].va)
      break;

  if(md == NOMAPS)
    return -1;

  p->mfile[md].va = p->sz;
  p->mfile[md].writable = p->ofile[fd]->writable;
  p->mfile[md].ip = p->ofile[fd]->ip;
  p->mfile[md].size = p->mfile[md].ip->size;
  
  p->mfile[md].ip->ref++;
  p->sz += PGROUNDUP(p->mfile[md].size);

  return p->mfile[md].va;
}

int
loadblock(struct proc *p, int md, uint64 va)
{
  char *pa;
  // TODO: Ask why we need to set read perm, if we don't set it, a panic: remap will occur.
  // In riscv priv docs, the scause 15 is an store or AMO. What AMO means?
  // Seeing uvmalloc implementation, it always turns on the read bit.
  if((pa = kalloc()) == 0)
    return -1;
  uint64 a = PGROUNDDOWN(va);
  if(mappages(p->pagetable, a, PGSIZE, (uint64)pa, PTE_R | PTE_W | PTE_U) == -1)
    return -1;
  int offset = a - p->mfile[md].va;
  ilock(p->mfile[md].ip);
  readi(p->mfile[md].ip, 1, a, offset, PGSIZE);
  iunlock(p->mfile[md].ip);
  return 0;
}

void
savechanges(struct inode* ip, uint64 va, int offset, int n)
{
  begin_op();
  ilock(ip);
  writei(ip, 1, va, offset, n);
  iunlock(ip);
  end_op();
}

void
applymodif(struct mapfile *mf, pagetable_t pagetable, uint64 va) {
  for(int offset = 0; offset < mf->size; offset += PGSIZE) {
    pte_t *pte = walk(pagetable, va + offset, 0);
    if(!(PTE_D & (*pte)))  // Dirty bit is zero
      continue;
    int a = va + offset;
    if(offset + PGSIZE > mf->size) // Last page
      savechanges(mf->ip, a, offset, mf->size - offset);
    else
      savechanges(mf->ip, a, offset, PGSIZE);
  }
}
