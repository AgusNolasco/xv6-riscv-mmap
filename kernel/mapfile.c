#include "types.h"
#include "param.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"

int
getmd(uint64 addr)
{
  struct proc *p = myproc();
  uint64 va, fsize;

  for (int md = 0; md < NOMAPS; md++) {
    va = p->mfile[md].va;
    if(!va)
      continue;
    fsize = p->ofile[p->mfile[md].fd]->ip->size;
    if(va <= addr && addr < va + PGROUNDUP(fsize))
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
    return 0;
  
  p->mfile[md].va = p->sz;
  p->mfile[md].perm = PTE_W;  //TODO: see what should be assigned
  p->mfile[md].fd = fd;
  int fsize = p->ofile[fd]->ip->size;
  printf("file size: %d, proc size: %d\n", fsize, p->sz);
  p->sz += PGROUNDUP(fsize);
  printf("updated proc size: %d\n", p->sz);
  
  return p->mfile[md].va;
}

int
loadblock(struct proc *p, int md, uint64 va)
{
  char *pa;
  if((pa = kalloc()) == 0)
    return -1;
  int perm = p->mfile[md].perm | PTE_R | PTE_U;
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
checkmodif(int fsize, struct inode *ip, pagetable_t pagetable, uint64 va) {
  for(int offset = 0; offset < fsize; offset += PGSIZE) {
    pte_t *pte = walk(pagetable, va + offset, 0);
    if(!(PTE_D & (*pte))){  // Dirty bit is zero
      printf("Ignoring changes from pte: %p\n", pte);
      continue;
    }
    int a = va + offset;
    if(offset + PGSIZE > fsize) // Last page
      savechanges(ip, a, offset, fsize - offset);
    else
      savechanges(ip, a, offset, PGSIZE);
  }
}
