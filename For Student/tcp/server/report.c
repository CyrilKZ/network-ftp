#include "server.h"
int reportLine(char* msg){
  printf("%s", msg);
}

void message_client(Session* ssn)
{
  write(ssn->connection, ssn->msgToClient, strlen(ssn->msgToClient));
}