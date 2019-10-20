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

	//����socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
	{
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	//����Ŀ��������ip��port
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = 10020;
	if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) <= 0)
	{ //ת��ip��ַ:���ʮ����-->������
		printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}
	//������Ŀ����������socket��Ŀ���������ӣ�-- ��������
	if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		printf("Error connect(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}
	while (1)
	{
		//��ȡ��������
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
			int n = write(sockfd, sentence, len); //write��������֤���е�����д�꣬������;�˳�
			if (n < 0)
			{
				printf("Error write(): %s(%d)\n", strerror(errno), errno);
				printf("Error1\n");
				return 1;
			}
			printf("Quiting\n");
			break;
		}
		//�Ѽ�������д��socket
		p = 0;
		while (p < len)
		{
			int n = write(sockfd, sentence + p, len + 1 - p); //write��������֤���е�����д�꣬������;�˳�
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
	//ե��socket���յ�������
	p = 0;
	while (1)
	{
		int n = read(sockfd, sentence + p, 8191 - p);
		if (n < 0)
		{
			printf("Error read(): %s(%d)\n", strerror(errno), errno); //read����֤һ�ζ��꣬������;�˳�
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

	//ע�⣺read�����Ὣ�ַ�������'\0'����Ҫ�ֶ�����
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