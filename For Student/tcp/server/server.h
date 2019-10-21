#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>

#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#define ARG_SIZE 1024
#define CONNECTED 0
#define USERNAME_OK 1
#define PASSWORD_OK 2
#define PAGE_SIZE 8192

int reportLine(char *str);
void error_out(char *str);



//server tool
typedef struct ipaddr {
  int h1;
  int h2;
  int h3;
  int h4;
} IpAddr;

typedef struct sockport {
  int p1;
  int p2;
} SockPort;

typedef struct comand
{
  char title[5];
  char arg[ARG_SIZE];
} Command;

typedef struct session
{
  //login info
  int authStage;
  char *loginPsw;

  char *msgToClient;

  //tcp connection
  int connection;

  //data connection
  int datafd;
  int datapid;

  int rcvMode;

  //socket for PASV mode
  int pasvfd;

  //addr of PORT mode
  IpAddr rcvAddr;
  SockPort rcvPort;

  //process id
  int pid;

  //file status
  int ascii;
  off_t currentPos;
  char* rnfrName;
  int aborFlag;
  char* currentDir;

} Session;

typedef enum cmdlist
{
  USER,
  PASS,
  RETR,
  STOR,
  QUIT,
  ABOR,
  SYST,
  TYPE,
  PORT,
  PASV,
  MKD,
  CWD,
  PWD,
  LIST,
  RMD,
  RNFR,
  RNTO
} CmdList;

IpAddr ip_of(int sock);
SockPort random_port();
typedef enum modes {
  PASSIVE, INPORT, NOMODE
} Modes;

//establish connection
int init_socket_atport(int port);
int init_connection_at(int port, InAddr ip);
int accept_connection(int);
int message_client(Session *);
int create_socket(int);

//command handler
static const char *CmdListText[] = {
    "USER", "PASS", "RETR", "STOR", "QUIT", "ABOR", "SYST", "TYPE", 
    "PORT", "PASV", "MKD", "CWD", "PWD", "LIST", "RMD", "RNFR", "RNTO"};

typedef struct threadparam {
  Command* cmd;
  Session* ssn;
} ThreadParam;

void cmd_user(Command *, Session *);
void cmd_pass(Command *, Session *);
void cmd_retr(Command *, Session *);
void cmd_stor(Command *, Session *);
void cmd_quit(Session *);
void cmd_syst(Command *, Session *);
void cmd_type(Command *, Session *);
void cmd_port(Command *, Session *);
void cmd_pasv(Command *, Session *);
void cmd_mkd(Command *, Session *);
void cmd_cwd(Command *, Session *);
void cmd_pwd(Command *, Session *);
void cmd_list(Command *, Session *);
void cmd_rmd(Command *, Session *);
void cmd_rnfr(Command *, Session *);
void cmd_rnto(Command *, Session *);



void parse_command(char *, Command *);
void handle_command(Command*);

int try_retr_connection(Session*);
void translate_todir(char* buffer, char* filename, Session*);
void reset_aborflag(Session* );