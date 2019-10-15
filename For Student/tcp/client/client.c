#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>

#include <string.h>
#include <memory.h>
#include <stdio.h>

int run(int argc, char **argv)
{
	int sockfd;
	struct sockaddr_in addr;
	char sentence[8192];
	int len;
	int p;

	//创建socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
	{
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	//设置目标主机的ip和port
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = 10020;
	if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) <= 0)
	{ //转换ip地址:点分十进制-->二进制
		printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}
	//连接上目标主机（将socket和目标主机连接）-- 阻塞函数
	if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		printf("Error connect(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}
	while (1)
	{
		//获取键盘输入
		memset(sentence, 0, 4096);
		fgets(sentence, 4096, stdin);
		len = strlen(sentence);
		if (sentence[len - 1] == '\n')
		{
			--len;
		}
		sentence[len] = '\0';
		fflush(stdin);
		if(strcmp(sentence, "quit") == 0)
		{
			sentence[0] = '\n';
			sentence[1] = '\0';
			len = 1;
			int n = write(sockfd, sentence, len); //write函数不保证所有的数据写完，可能中途退出
			if (n < 0)
			{
				printf("Error write(): %s(%d)\n", strerror(errno), errno);
				printf("Error1\n");
				return 1;
			}
			printf("Quiting\n");
			break;
		}
		//把键盘输入写入socket
		p = 0;
		while (p < len)
		{
			int n = write(sockfd, sentence + p, len + 1 - p); //write函数不保证所有的数据写完，可能中途退出
			if (n < 0)
			{
				printf("Error write(): %s(%d)\n", strerror(errno), errno);
				printf("Error1\n");
				return 1;
			}
			else
			{
				p += n;
			}
		}
		printf("Send message: %s\n", sentence);
	}
	//榨干socket接收到的内容
	p = 0;
	while (1)
	{
		int n = read(sockfd, sentence + p, 8191 - p);
		if (n < 0)
		{
			printf("Error read(): %s(%d)\n", strerror(errno), errno); //read不保证一次读完，可能中途退出
			printf("Error2\n");
			return 1;
		}
		else if (n == 0)
		{
			break;
		}
		else
		{
			p += n;
			if (sentence[p - 1] == '\n')
			{
				break;
			}
			if (sentence[p - 1] == '\0')
			{
				printf("Segment Received\n");
				--p;
			}
		}
	}

	//注意：read并不会将字符串加上'\0'，需要手动添加
	sentence[p - 1] = '\0';
	printf("Received message: %s\n", sentence);
	close(sockfd);

	return 0;
}

int main(int argc, char **argv)
{
	int res = run(argc, argv);
	printf("exit with code %d\n", res);
}