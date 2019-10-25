#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <sys/wait.h>
#include <netinet/in.h>

#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <dirent.h>




int main(){
  chroot("/tmp");
  chdir("/");
  rename("test", "tmp");
}