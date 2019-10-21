#include "server.h"
int reportLine(char* msg){
  printf("%s", msg);
}

void message_client(Session* ssn)
{
  int len = strlen(ssn->msgToClient);
  char buffer[4096] = {0};
  strcpy(buffer, ssn->msgToClient);
  if(buffer[len - 1] == '\n')
  {
    buffer[len - 1] = '\r';
    buffer[len] = '\n';
  }
  else
  {
    buffer[len] = '\r';
    buffer[len + 1] = '\n';
  }
  len = strlen(buffer);
  int n = write(ssn->connection, buffer, len);
  printf("Write to client MSG=<%s>, N=<%d>\n",buffer, n);
}
