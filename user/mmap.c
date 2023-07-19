#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/riscv.h"

int main()
{
  int fd = open("hello.txt", O_RDWR);
  printf("fd = %d\n", fd);

  for(int i = -2; i < fd; i++)
    printf("fd %d -> %p\n", i, mmap(i, 0));

  char *addr, *addr2;
  printf("addr: %p\n", addr = mmap(fd, PTE_R | PTE_W));
  printf("addr2: %p\n", addr2 = mmap(fd, 0));
  printf("mmap: %d\n", addr);
  printf("file[0]: %c\n", addr[0]);
  printf("file[1]: %c\n", addr[1]);
  printf("file[2]: %c\n", addr[2]);
  printf("file[3]: %c\n", addr[3]);
  printf("file[4]: %c\n", addr[4]);
  printf("file[4095]: %c\n", addr[4095]);
  addr[5] = 'c';
  addr[6] = '\n';
  printf("munmap: %d\n", munmap(addr));
  // printf("file[0]: %c\n", addr[0]); // this produce a pagefault as expected
  printf("munmap: %d\n", munmap(addr));
  return 0;
}
