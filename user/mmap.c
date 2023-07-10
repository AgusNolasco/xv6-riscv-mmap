#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int main()
{
  int fd = open("hello.txt", O_RDWR);
  char* addr = mmap(fd);
  printf("mmap: %d\n", addr);
  printf("file[0]: %c\n", addr[0]);
  printf("file[1]: %c\n", addr[1]);
  printf("file[2]: %c\n", addr[2]);
  printf("file[3]: %c\n", addr[3]);
  printf("file[4]: %c\n", addr[4]);
  printf("file[4095]: %c\n", addr[4095]);
  printf("file[4096]: %c\n", addr[4096]);
  printf("file[4097]: %c\n", addr[4097]);
  printf("munmap: %d\n", munmap(addr));
  return 0;
}
