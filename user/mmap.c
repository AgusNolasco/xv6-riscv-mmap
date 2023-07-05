#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int main()
{
  int fd = open("hello.txt", O_CREATE);
  char* addr = mmap(fd);
  printf("mmap: %d\n", addr);
  printf("munmap: %d\n", munmap(addr));
  return 0;
}
