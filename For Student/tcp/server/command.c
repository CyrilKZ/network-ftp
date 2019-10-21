#include "server.h"
void stringfy_commandline(char* dest, char* raw)
{
  int len = strlen(raw);
  while(len > 0 && (raw[len - 1] == '\n' || raw[len - 1] == '\r'))
  {
    raw[len - 1] = 0;
    --len;
  }
  return;
}

void parse_command(char *cmdline, Command *cmdobj)
{
  sscanf(cmdline, "%s %s", cmdobj->title, cmdobj->arg);
}

void handle_command(Command *cmd, Session *ssn)
{
  /* determine what is the command */
  int cmdIdx = -1;
  int cmdCount = sizeof(CmdListText) / sizeof(char *);
  for (int i = 0; i < cmdCount; ++i)
  {
    if (strcmp(CmdListText[i], cmd->title) == 0)
    {
      cmdIdx = i;
      break;
    }
  }
  switch (cmdIdx)
  {
  case USER:
    cmd_user(cmd, ssn);
    break;
  case PASS:
    cmd_pass(cmd, ssn);
    break;
  case RETR:
    cmd_retr(cmd, ssn);
    break;
  case STOR:
    cmd_stor(cmd, ssn);
    break;
  case REST:
    cmd_rest(cmd, ssn);
    break;
  case QUIT:
    cmd_quit(cmd, ssn);
    break;
  case SYST:
    cmd_syst(ssn);
    break;
  case TYPE:
    cmd_type(cmd, ssn);
    break;
  case PORT:
    cmd_port(cmd, ssn);
    break;
  case PASV:
    cmd_pasv(cmd, ssn);
    break;
  case MKD:
    cmd_mkd(cmd, ssn);
    break;
  case CWD:
    cmd_cwd(cmd, ssn);
    break;
  case PWD:
    cmd_pwd(cmd, ssn);
    break;
  case LIST:
    cmd_list(cmd, ssn);
    break;
  case RMD:
    cmd_rmd(cmd, ssn);
    break;
  case RNFR:
    cmd_rnfr(cmd, ssn);
    break;
  case DELE:
    cmd_dele(cmd, ssn);
    break;
  case RNTO:
    cmd_rnto(cmd, ssn);
    break;
  default:
    ssn->msgToClient = "500 Invalid command.\n";
    message_client(ssn);
    break;
  }
}

void cmd_user(Command* cmd, Session* ssn)
{
  if(ssn->authStage >= USERNAME_OK)
  {
    ssn->msgToClient = "502 Guest already logged in.\n"
  }
  else if(strcmp(cmd->arg,"anonymous"))
  {
    ssn->msgToClient = "331 Guest login ok, send your complete e-mail address as password.\n";
    ssn->authStage = USERNAME_OK; 
  }
  else
  {
    ssn->authStage = CONNECTED;
    ssn->msgToClient = "530 Guest login failed, invalid username.\n";
  }
  message_client(ssn);
}

void cmd_pass(Command* cmd, Session* ssn)
{
  if(ssn->authStage >= PASSWORD_OK)
  {
    ssn->msgToClient = "502 Guest already logged in.\n";
  }
  else if(ssn->authStage == USERNAME_OK)
  {
    ssn->loginPsw = (char*)malloc((strlen(cmd->arg) + 1) * sizeof(char));
    strcpy(ssn->loginPsw, cmd->arg);
    ssn->msgToClient = "230 Guest login ok, welcome to this FTP sever!\n";
  }
  else
  {
    ssn->msgToClient = "530 Guest login failed, please send USER first.\n";
  }
  message_client(ssn);
}

void cmd_pasv(Command* cmd, Session* ssn)
{
  if(ssn->authStage != PASSWORD_OK)
  {
    ssn->msgToClient = "530 Please login.\n";
    message_client(ssn);
    return;
  }
  if(ssn->rcvMode != NOMODE)
  {
    ssn->msgToClient = "502 Mode already selected.\n";
    message_client(ssn);
    return;
  }
  char msg[1024] = {0};
  IpAddr ip = ip_of(ssn->connection);
  SockPort port = random_port();
  sprintf(msg, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d,%d,%d).\n", ip.h1, ip.h2, ip.h3, ip.h4, port.p1, port.p2);
  ssn->msgToClient = msg;
  ssn->mode = PASSIVE;
  if((ssn->pasvfd = init_socket_atport(port.p1 * 256 + port.p2)) == -1)
  {
    ssn->msgToClient = "452 Internal error";
  }
  message_client(ssn);
}

void cmd_port(Command* cmd, Session* ssn)
{
  if(ssn->authStage != PASSWORD_OK)
  {
    ssn->msgToClient = "530 Please login.\n";
    message_client(ssn);
    return;
  }
  if(ssn->rcvMode != NOMODE)
  {
    ssn->msgToClient = "502 Mode already selected.\n";
    message_client(ssn);
    return;
  }
  IpAddr addr;
  SockPort port;
  sscanf(cmd->arg, "%d,%d,%d,%d,%d,%d", &addr.h1, &addr.h2, &addr.h3, &addr.h4, &port.p1, &port.p2);
  ssn->rcvAddr = addr;
  ssn->rcvPort = port;
  ssn->msgToClient = "220 PORT command successful.\n";
  message_client(ssn);
}

int try_retr_connection(Session* ssn)
{
  if(ssn->rcvMode == PASSIVE)
  {
    if((ssn->datafd = accept_connection(ssn->pasvfd)) == -1)
    {
      ssn->msgToClient = "426 Cannot open connection.\n";
      message_client(ssn);
      return -1;
    }
  }
  else
  {
    if((ssn->datafd = init_connection_at(ssn->rcvPort.p1 * 256 + ssn->rcvPort.p2, ssn->rcvAddr)) == -1)
    {
      ssn->msgToClient = "426 Cannot open connection.\n";
      message_client(ssn);
      return -1;
    }
  }
}

void cmd_retr(Command* cmd, Session* ssn)
{
  if(ssn->authStage != PASSWORD_OK)
  {
    ssn->msgToClient = "530 Please login.\n";
    message_client(ssn);
    return;
  }
  if(ssn->rcvMode == NOMODE)
  {
    ssn->msgToClient = "550 Please select PORT or PASV mode.\n";
    message_client(ssn);
    return;
  }
  char buffer[2048];
  if(translate_todir(buffer, cmd->arg, ssn) == -1)
  {
    ssn->msgToClient = "551 Filename too long";
    message_client(ssn);
    return;
  }
  int fd = open(buffer, O_RDONLY);
  if(fd == -1)
  {
    ssn->msgToClient = "551 Cannot open file.\n";
    message_client(ssn);
    return;
  }
  struct stat buf;
  fstat(fd, &buf);
  off_t total = buf.st_size;
  off_t offset = ssn->currentPos;
  off_t remain = 0;
  if(lseek(fd, offset, SEEK_SET) == -1)
  {
    ssn->msgToClient = "551 Cannot open file.\n";
    message_client(ssn);
    return;
  }
  if(try_retr_connection(ssn) == -1)
  {
    return;
  }
  memset(buffer, 0, 2048);
  sprintf(buffer, "150 Opening BINARY mode data connection for %s (%ld bytes).\n", cmd->arg, total);
  ssn->msgToClient = buffer;
  if(offset > total)
  {
    remain = 0;
  }
  else
  {
    remain = total - offset;
  }
  int status = 0;
  message_client(ssn);
  while(remain)
  {
    int singleTimeTotal = remain > PAGE_SIZE ? PAGE_SIZE : remain;
    int n = sendfile(ssn->datafd, fd, NULL, singleTimeTotal);
    if(n == -1)
    {
      status = 1;
      break;
    }
    if(ssn->aborFlag)
    {
      status = 2;
      break;
    }
    remain -= n;
  }
  if(remain == 0)
  {
    status = 0;
  }
  close(ssn->datafd);
  ssn->datafd = -1;
  close(fd);
  close(ssn->pasvfd);
  ssn->pasvfd = -1;
  ssn->rcvMode = NOMODE;
  switch (status)
  {
  case 0:
    ssn->msgToClient = "226 Transfer complete.\n";
    break;
  case 1:
    ssn->msgToClient = "426 Fail to transfer file.\n";
    break;
  case 2:
    ssn->msgToClient = "450 Transfer interrupted.\n";
  default:
    break;
  }
  message_client(ssn);
}

void cmd_stor(Command* cmd, Session* ssn)
{
  if(ssn->authStage != PASSWORD_OK)
  {
    ssn->msgToClient = "530 Please login.\n";
    message_client(ssn);
    return;
  }
  if(ssn->rcvMode == NOMODE)
  {
    ssn->msgToClient = "550 Please select PORT or PASV mode.\n";
    message_client(ssn);
    return;
  }  
  char buffer[2048];
  if(translate_todir(buffer, cmd->arg, ssn) == -1)
  {
    ssn->msgToClient = "551 Filename too long";
    message_client(ssn);
    return;
  }
  int fd = open(buffer, O_CREAT | O_WRONLY, 0666);
  if(fd == -1)
  {
    ssn->msgToClient = "551 Cannot open file.\n";
    message_client(ssn);
    return;
  }
  ftruncate(fd, 0);
  if (lseek(fd, 0, SEEK_SET) < 0)
  {
    ssn->msgToClient = "551 Cannot open file.\n";
    message_client(ssn);
    return;
  }
  struct stat buf;
  fstat(fd, &buf);
  off_t total = buf.st_size;
  off_t offset = ssn->currentPos;
  off_t remain = 0;
  if(lseek(fd, offset, SEEK_SET) == -1)
  {
    ssn->msgToClient = "551 Cannot open file.\n";
    message_client(ssn);
    return;
  }
  if(try_retr_connection(ssn) == -1)
  {
    return;
  }
  memset(buffer, 0, 2048);
  sprintf(buffer, "150 Opening BINARY mode data connection for %s (%ld bytes).\n", cmd->arg, total);
  ssn->msgToClient = buffer;
  int status = 0;
  message_client(ssn); 
  while(1)
  {
    memset(buffer, 0, 2048);
    int n = read(ssn->datafd, fd, sizof(buffer));
    if(n == -1)
    { 
      if(errno == EINTR)
      {
        continue;
      }
      else
      {
        status = 1;
        break;
      }  
    }
    if(ssn->aborFlag)
    {
      status = 2;
      break;
    }
    if(write(fd, buffer, n) != n)
    {
      status = 1;
      break;
    }
  }
  close(ssn->datafd);
  ssn->datafd = -1;
  close(fd);
  close(ssn->pasvfd);
  ssn->pasvfd = -1;
  ssn->rcvMode = NOMODE;
  switch (status)
  {
  case 0:
    ssn->msgToClient = "226 Transfer complete.\n";
    break;
  case 1:
    ssn->msgToClient = "426 Fail to transfer file.\n";
    break;
  case 2:
    ssn->msgToClient = "450 Transfer interrupted.\n";
  default:
    break;
  }
  message_client(ssn);
}

void cmd_type(Command* cmd, Session* ssn)
{
  if(strcmp(cmd->arg, "I") != 0)
  {
    ssn->msgToClient = "500 type not supported.\n";
    message_client(ssn);
    return;
  }
  else
  {
    ssn->ascii = 0;
  }
}

void cmd_cwd(Command* cmd, Session* ssn)
{
  if(chdir(cmd->arg) < 0)
  {
    ssn->msgToClient = "550 Fail to change directory.\n";
  }
  else
  {
    ssn->msgToClient = "250 Directory change successful.\n";
  }
  message_client(ssn);
}

void cmd_mkd(Command* cmd, Session* ssn)
{
  if(mkdir(cmd->arg, 0777) < 0)
  {
    ssn->msgToClient = "550 Fail to create directory.\n";
    message_client(ssn);
    return;
  }
  char dir[1024] = {0};
  char text[2048] = {0};
  if(cmd->arg[0] == '/')
  {
    sprintf(text, "257 %s created.\n");
  }
  else
  {
    getcwd(dir, 4096);
    if(dir[strlen(dir)-1] == '/')
    {
      sprintf(text, "257 %s%s created.\n", dir, cmd->arg);
    }
    else
    {
      sprintf(text, "257 %s/%s created.\n", dir, cmd->arg);
    }
  }
  message_client(ssn);
  return;
}

void cmd_rmd(Command* cmd, Session* ssn)
{
  if(rmdir(cmd->arg) < 0)
  {
    ssn->msgToClient = "550 Fail to remove directory.\n";
  }
  else
  {
    ssn->msgToClient = "250 Directory remove operation successful.\n"; 
  }
  message_client(ssn);
}

void cmd_dele(Command* cmd, Session* ssn)
{
  if(unlink(cmd->arg) < 0)
  {
    ssn->msgToClient = "550 Fail to delete file.\n";
  }
  else
  {
    ssn->msgToClient = "250 File remove operation successful.\n"; 
  }
  message_client(ssn); 
}

void cmd_pwd(Command* cmd, Session* ssn)
{
  char text[2048] = {0};
  char dir[1024] = {0};
  getcwd(dir, 1023);
  sprintf(text, "\"%s\"", dir);
  ssn->msgToClient = dir;
  message_client(ssn);
}

void cmd_rnfr(Command* cmd, Session* ssn)
{
  if(strlen(cmd->arg) >= 256)
  {
    ssn->msgToClient = "550 RNFR name too long.\n"
    message_client(ssn);
    return;
  }
  if(ssn->rnfrName == NULL)
  {
    ssn->rnfrName = (char*)malloc(256*sizeof(char));
  }
  memset(ssn->rnfrName, 0 , 256);
  strcpy(ssn->rnfrName, cmd->arg);
  ssn->msgToClient = "350 Ready for RNTO.\n";
  message_client(ssn);
}

void cmd_rnto(Command* cmd, Session* ssn)
{
  if(strlen(cmd->arg) >= 256)
  {
    ssn->msgToClient = "550 RNTO name too long.\n"
    message_client(ssn);
    return;
  }
  if(ssn->rnfrName == NULL)
  {
    ssn->msgToClient = "503 Use RNFR first.\n";
    message_client(ssn);
    return;
  }
  if(rename(ssn->rnfrName, cmd->arg) == -1)
  {
    ssn->msgToClient = "550 Fail to rename file or directory.\n";
  }
  else
  {
    ssn->msgToClient = "250 Rename succesfull.\n";
  }
  message_client(ssn);
  return;
}

void cmd_rest(Command* cmd, Session* ssn)
{
  ssn->currentPos = atoi(cmd->arg);
  char msg[1024] = {0};
  sprintf(msg, "350 Restart position accepted (%ld)", (long)ssn->currentPos);
  ssn->msgToClient = msg;
  message_client(ssn);
}

void cmd_syst(Session* ssn)
{
  ssn->msgToClient = "215 UNIX type: L8.\n";
  message_client(ssn);
  return;
}

void cmd_quit(Session* ssn)
{
  ssn->msgToClient = "221 Goodbye.\n";
  message_client(ssn);
  close(ssn->connection);
  exit(0);
}

void reset_aborflag(Session* ssn)
{
  if(ssn->aborFlag)
  {
    ssn->aborFlag = 0;
  }
}