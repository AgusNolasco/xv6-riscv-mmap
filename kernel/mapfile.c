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
  struct file *f;
  uint64 va, fsize;

  for (int md = 0; md < NOMAPS; md++) {
    va = p->mfile[md].va;
    if(!va)
      continue;
    f = p->ofile[p->mfile[md].fd];
    if(!f)
      continue;
    fsize = f->ip->size;
    if(va <= addr && addr < va + PGROUNDUP(fsize))
      return md;
  }
  return -1;
}

int
mfilealloc(struct proc *p, int fd, int perm)
{
  int md;
  for(md = 0; md < NOMAPS; md++)
    if(!p->mfile[md].va)
      break;

  if(md == NOMAPS)
    return -1;

  if((perm & PROT_READ) && !p->ofile[fd]->readable)
    return -1;
  if((perm & PROT_WRITE) && !p->ofile[fd]->writable)
    return -1;

  p->mfile[md].va = p->sz;
  p->mfile[md].perm = perm;
  p->mfile[md].fd = fd;
  int fsize = p->ofile[fd]->ip->size;
  p->sz += PGROUNDUP(fsize);

  return p->mfile[md].va;
}

static int
isvalidperm(int perm, int cause)
{
  if(cause == WRITE && (perm & PROT_WRITE))
    return 1;
  if(cause == READ && (perm & PROT_READ))
    return 1;
  return 0;
}

int
loadblock(struct proc *p, int md, uint64 va, int cause)
{
  char *pa;
  int perm = p->mfile[md].perm | PTE_R | PTE_U;
  // TODO: Ask why we need to set read perm, if we don't set it, a panic: remap will occur.
  // In riscv priv docs, the scause 15 is an store or AMO. What AMO means?
  // Seeing uvmalloc implementation, it always turns on the read bit.
  if(!isvalidperm(perm, cause))
    return -1;
  if((pa = kalloc()) == 0)
    return -1;
  uint64 a = PGROUNDDOWN(va);
  if(mappages(p->pagetable, a, PGSIZE, (uint64)pa, perm) == -1)
    return -1;
  int fd = p->mfile[md].fd;
  int offset = a - p->mfile[md].va;
  ilock(p->ofile[fd]->ip);
  readi(p->ofile[fd]->ip, 1, a, offset, PGSIZE);
  iunlock(p->ofile[fd]->ip);
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
checkmodif(struct inode *ip, pagetable_t pagetable, uint64 va) {
  for(int offset = 0; offset < ip->size; offset += PGSIZE) {
    pte_t *pte = walk(pagetable, va + offset, 0);
    if(!(PTE_D & (*pte)))  // Dirty bit is zero
      continue;
    int a = va + offset;
    if(offset + PGSIZE > ip->size) // Last page
      savechanges(ip, a, offset, ip->size - offset);
    else
      savechanges(ip, a, offset, PGSIZE);
  }
}
