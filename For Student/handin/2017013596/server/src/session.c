#include "server.h"

Session new_session(int fd)
{
  Session ssn;
  memset(&ssn, 0, sizeof(ssn));
  ssn.connection = fd;
  ssn.datafd = -1;
  ssn.pasvfd = -1;
  ssn.rcvMode = NOMODE;
  return ssn;
}

Command new_command()
{
  Command cmd;
  memset(&cmd, 0, sizeof(cmd));
  return cmd;
}