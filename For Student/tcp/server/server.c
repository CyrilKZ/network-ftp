#include "server.h"

char rootDirectory[512] = "";
int portNum = 21;

void error_out(char *err)
{
	printf("%s", err);
	exit(1);
}

void get_params(int argc, char **argv)
{
	char errMsg[40] = "Error: Invalid initial parameters\n";
	char cwd[255];
	char temp[256] = "tmp";
	if (getcwd(cwd, sizeof(cwd)) == NULL)
	{
		error_out("Error: Work directory invalid\n");
	}
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
			strncpy(temp, argv[2], 200);
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
				strncpy(temp, argv[4], 200);
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
	strncpy(rootDirectory, cwd, 255);
	strcat(rootDirectory, "/");
	strcat(rootDirectory, temp);
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
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return -1;
	}
	if(bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
	{
		printf("Error bind(): %s(%d)\n", strerror(errno), errno);
		return -1;
	}
	if (listen(sock, 10) == -1)
	{
		printf("Error listen(): %s(%d)\n", strerror(errno), errno);
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
		printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
		return -1;
	}
	int sock;
	if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
	{
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return -1;
	}
	if(connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		printf("Error connect(): %s(%d)\n", strerror(errno), errno);
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

int translate_todir(char* buffer, char*filename, Session* ssn)
{
	if(strlen(ssn->currentDir) > 1024 || strlen(filename) > 1024)
	{
		return -1;
	}
	strcpy(buffer, ssn->currentDir);
	strcat(buffer, filename);
	return 1;
}

int main(int argc, char **argv)
{
	get_params(argc, argv);

	int listenfd, connfd; //监听socket和连接socket不一样，后者用于数据传输
	struct sockaddr_in addr;
	char sentence[8192];
	int p;
	int len;

	listenfd = init_socket_atport(portNum);
	if(listenfd == -1)
	{
		return 1;
	}

	printf("Server running ...\n");
	printf("File root directory:\t%s\n", rootDirectory);
	printf("Listening port: \t%d\n", portNum);

	//持续监听连接请求
	while (1)
	{
		memset(sentence, 0, 8192);
		//等待client的连接 -- 阻塞函数
		if ((connfd = accept(listenfd, NULL, NULL)) == -1)
		{
			printf("Error accept(): %s(%d)\n", strerror(errno), errno);
			continue;
		}	
		p = 0;
		while (1)
		{
			//榨干socket传来的内容
			int n = read(connfd, sentence + p, 8191 - p);
			if (n < 0)
			{
				printf("Error read(): %s(%d)\n", strerror(errno), errno);
				close(connfd);
				continue;
			}
			else if (n == 0)
			{
				printf("Message Ended\n");
				break;
			}
			else
			{
				p += n;
				if (sentence[p - 1] == '\0')
				{
					printf("Segment Received\n");
					--p;
				}
				if (sentence[p - 1] == '\n')
				{
					printf("Message Ended\n");
					break;
				}
			}
		}
		//socket接收到的字符串并不会添加'\0'
		sentence[p] = '\0';
		len = p;

		printf("Received message: %s\n", sentence);

		//字符串处理
		for (p = 0; p < len; p++)
		{
			sentence[p] = toupper(sentence[p]);
		}

		//发送字符串到socket
		p = 0;
		while (p < len)
		{
			int n = write(connfd, sentence + p, len + 1 - p);
			if (n < 0)
			{
				printf("Error write(): %s(%d)\n", strerror(errno), errno);
				return 1;
			}
			else
			{
				p += n;
			}
		}
		printf("Send message: %s\n", sentence);
		close(connfd);
	}
	close(listenfd);
}
