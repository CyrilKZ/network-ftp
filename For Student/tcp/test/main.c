#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

int main(){
  char buff[200] = {0};
  getcwd(buff, 200);
  strcat(buff, "/temp");
  printf("%s\n", buff);
  printf("%d\n", chroot(buff));
  memset(buff, 0, 200);
  getcwd(buff, 200);
  printf("%s\n", buff);
  printf("%d\n", chdir("/temp2"));
  memset(buff, 0, 200);
  getcwd(buff, 200);
  printf("%s\n", buff);
  //int m = mkdir("/temp2", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  //printf("%d\n", m);
  int i = 0;
  while(1){
    ++i;
  }
}