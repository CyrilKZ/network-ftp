#include "server.h"

char rootDirectory[512] = "";
int portNum = 21;

void error_out(char *err)
{
	///* deletelater */printf("%s", err);
	exit(1);
}

void get_params(int argc, char **argv)
{
	char errMsg[40] = "Error: Invalid initial parameters\n";
	char temp[256] = {0};
	if (argc != 1 && argc != 3 && argc != 5)
	{
		error_out(errMsg);
	}
	if (argc == 3)
	{
		if (strcmp("-root", argv[1]) == 0)
		{
			strncpy(temp, argv[2], 256);
		}
		else if (strcmp("-port", argv[1]) == 0)
		{
			portNum = atoi(argv[2]);
			if (portNum <= 0)
			{
				error_out(errMsg);
			}
		}
		else
		{
			error_out(errMsg);
		}
	}
	if (argc == 5)
	{
		if (strcmp("-root", argv[1]) == 0)
		{
			strncpy(temp, argv[2], 256);
			if (strcmp("-port", argv[3]) == 0)
			{
				portNum = atoi(argv[4]);
				if (portNum <= 0)
				{
					error_out(errMsg);
				}
			}
			else
			{
				error_out(errMsg);
			}
		}
		else if (strcmp("-port", argv[1]) == 0)
		{
			portNum = atoi(argv[2]);
			if (portNum <= 0)
			{
				error_out(errMsg);
			}
			if (strcmp("-root", argv[3]) == 0)
			{
				strncpy(temp, argv[4], 256);
			}
			else
			{
				error_out(errMsg);
			}
		}
		else
		{
			error_out(errMsg);
		}
	}
	if(temp[0] == 0)
	{
		strncpy(rootDirectory, "/tmp", 256);
	}
	else
	{
		strncpy(rootDirectory, temp, 256);
	}
	if (access(rootDirectory, 04) == -1)
	{
		if (mkdir(rootDirectory, 0777) == -1)
		{
			error_out("Error: Could not access root\n");
		}
	}
	if(chroot(rootDirectory) == -1)
	{
		error_out("Error: Could not set root\n");
	}
	if(chdir("/") == -1)
	{
		error_out("Error: Could not set root\n");
	}
}

IpAddr ip_of(int sock)
{
  struct sockaddr_in addrIn;
  socklen_t size = sizeof(addrIn);
  getsockname(sock, (struct sockaddr*)&addrIn, &size);
  int h = addrIn.sin_addr.s_addr;
  IpAddr ip;
  ip.h1 = h & 0xff;
  h >> 1;
  ip.h2 = h & 0xff;
  h >> 1;
  ip.h3 = h & 0xff;
  h >> 1;
  ip.h4 = h & 0xff;
  return ip;
}

SockPort random_port()
{
  SockPort res;
  srand(time(NULL));
  res.p1 = (rand()%64) + 128;
  res.p2 = rand()%256;
  return res;
}

int init_socket_atport(int port)
{
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	int sock;
	if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
	{
		///* deletelater */printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return -1;
	}
	if(bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
	{
		///* deletelater */printf("Error bind(): %s(%d)\n", strerror(errno), errno);
		return -1;
	}
	if (listen(sock, 10) == -1)
	{
		///* deletelater */printf("Error listen(): %s(%d)\n", strerror(errno), errno);
		return -1;
	}
	return sock;
}

int init_connection_at(int port, IpAddr ip)
{
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	char buffer[256] = {0};
	sprintf(buffer, "%d.%d.%d.%d", ip.h1, ip.h2, ip.h3, ip.h4);
	if(inet_pton(AF_INET, buffer, &addr.sin_addr) <= 0)
	{
		///* deletelater */printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
		return -1;
	}
	int sock;
	if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
	{
		///* deletelater */printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return -1;
	}
	if(connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		///* deletelater */printf("Error connect(): %s(%d)\n", strerror(errno), errno);
		return -1;
	}
	return sock;
}

int accept_connection(int socket)
{
	struct sockaddr_in clientAddr;
	int size = sizeof(clientAddr);
	return accept(socket, (struct sockaddr*)&clientAddr, &size);
}

int test_dir(char* buffer)
{
	int len = strlen(buffer);
	for(int i = 0; i < len - 2; ++i)
	{
		if(buffer[i] == '.' && buffer[i+1] == '.' && buffer[i+2] == '/')
		{
			return -1;
		}
	}
	return 0;
}
void server_wait(int signum)
{
  int status;
  wait(&status);
}

void communicate(int fd)
{
	char buffer[2048] = {0};
	Command cmd = new_command();
	Session ssn = new_session(fd);
	ssn.msgToClient = "220 Welcome to this FTP server, please login with USER command.\n";
	message_client(&ssn);
	int nread = 0;
	while(nread = read(fd, buffer, 4096))
	{
		if(nread < 0 || nread > 4096)
		{
			///* deletelater */printf("Error read() at communicate.\n");
			continue;
		}
		buffer[2047] = '\0';
		
		if(strlen(buffer) > 1024 + 6)
		{
			ssn.msgToClient = "500 Argument too long.\n";
			message_client(&ssn);
			continue;
		}
		stringfy_commandline(buffer);
		///* deletelater */printf("raw msg received: <%s>\n", buffer);
		if(buffer[0] > 127 || buffer[0] <= 0)
		{
			ssn.msgToClient = "500 Invalid command.\n";
			message_client(&ssn);
			continue;
		}
		parse_command(buffer, &cmd);
		///* deletelater */printf("Message Received: CMD=<%s> ARG=<%s>\n", cmd.title, cmd.arg);
		handle_command(&cmd, &ssn);
		memset(&cmd, 0, sizeof(Command));
		memset(buffer, 0, sizeof(buffer));
	}
	if(ssn.rnfrName != NULL)
  {
    free(ssn.rnfrName);
  }
	///* deletelater */printf("Client disconnected\n");
}

int main(int argc, char **argv)
{
	get_params(argc, argv);
	int listenfd, connfd;
	struct sockaddr_in addr;
	char sentence[8192] = {0};
	int p;
	int len;

	listenfd = init_socket_atport(portNum);
	if(listenfd == -1)
	{
		return 1;
	}

	getcwd(sentence, 2048);
	///* deletelater */printf("Server running ...\n");
	///* deletelater */printf("File root directory:\t%s\n", rootDirectory);
	///* deletelater */printf("Cuurent directory:\t%s\n", sentence);
	///* deletelater */printf("Listening port: \t%d\n", portNum);
	while (1)
	{
		memset(sentence, 0, 8192);
		if ((connfd = accept(listenfd, NULL, NULL)) == -1)
		{
			///* deletelater */printf("Error accept(): %s(%d)\n", strerror(errno), errno);
			continue;
		}	

		int pid = fork();

		if(pid < 0)
		{
			char* res = "425 Cannot create Process for connection.\n";
			write(connfd, res, strlen(res));
			///* deletelater */printf("Error fork(): cannot open new process.\n");
			continue;
		}

		if(pid == 0)
		{
			signal(SIGCHLD, server_wait);
			///* deletelater */printf("Connected\n");
			listenfd = sclose_sock(listenfd);
			communicate(connfd);
			connfd = sclose_sock(connfd);
			exit(0);
		}
		else
		{
			connfd = sclose_sock(connfd);
		}
	}
	close(listenfd);
}

