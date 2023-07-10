#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int main()
{
  const char *filename = "testdata.txt"; 
  int f = open(filename, O_RDWR);

  if (f < 0) {
    const char *str1 = "Hello world in page 1";
    const char *str2 = "I'm in second page";
    const unsigned char zero = 0;
    int l = strlen(str1);
    int i;

    f = open(filename, O_RDWR | O_CREATE);
    write(f, str1, l);
    for (i=l; i<4096; i++)
        write(f, &zero, 1);
    write(f, str2, strlen(str2)+1);
  }

  char* addr = mmap(f);
  printf("mmap: %d\n", addr);
  printf("file[4096]: %c\n", addr[4096]);
  printf("file[0]: %c\n", addr[0]);
  printf("munmap: %d\n", munmap(addr));
  return 0;
}
