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
#include "server.h"


char rootDirectory[512] = "";
int portNum = 21;

void errorOut(char* err){
	printf("%s", err);
	exit(1);
}

void getParams(int argc, char **argv){
	char errMsg[40] = "Error: Invalid initial parameters\n";
	char cwd[255];
	char temp[256] = "tmp";
	if(getcwd(cwd, sizeof(cwd)) == NULL){
		errorOut("Error: Work directory invalid\n");
	}
	if(argc != 1 && argc != 3 && argc != 5){
		errorOut(errMsg);
	}
	if(argc == 3){
		if(strcmp("-root", argv[1]) == 0){			
			strncpy(temp, argv[2], 256);
		}
		else if(strcmp("-port", argv[1]) == 0){
			portNum = atoi(argv[2]);
			if(portNum <= 0){
				errorOut(errMsg);
			}
		}
		else{
			errorOut(errMsg);
		}
	}
	if(argc == 5){
		if(strcmp("-root", argv[1]) == 0){
			strncpy(temp, argv[2], 200);
			if(strcmp("-port", argv[3]) == 0){
				portNum = atoi(argv[4]);
				if(portNum <= 0){
					errorOut(errMsg);
				}
			}
			else{
				errorOut(errMsg);
			}
		}
		else if(strcmp("-port", argv[1]) == 0){
			portNum = atoi(argv[2]);
			if(portNum <= 0){
				errorOut(errMsg);
			}
			if(strcmp("-root", argv[3]) == 0){
				strncpy(temp, argv[4], 200);
			}
			else{
				errorOut(errMsg);
			}
		}
		else{
			errorOut(errMsg);
		}
	}
	strncpy(rootDirectory, cwd, 255);
	strcat(rootDirectory, "/");
	strcat(rootDirectory, temp);
	if(access(rootDirectory, 04) == -1){
		if(mkdir(rootDirectory, 0777) == -1){
			errorOut("Error: Could not access root\n");
		}
	}	
}

int main(int argc, char **argv) {
	getParams(argc, argv);


	int listenfd, connfd;		//����socket������socket��һ���������������ݴ���
	struct sockaddr_in addr;
	char sentence[8192];
	int p;
	int len;

	

	//����socket
	if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	//���ñ�����ip��port
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = portNum;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);	//����"0.0.0.0"

	//��������ip��port��socket��
	if (bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		printf("Error bind(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	//��ʼ����socket
	if (listen(listenfd, 10) == -1) {
		printf("Error listen(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	printf("Server running ...\n");
	printf("File root directory:\t%s\n", rootDirectory);
	printf("Listening port: \t%d\n", portNum);


	//����������������
	while (1) {
		//�ȴ�client������ -- ��������
		if ((connfd = accept(listenfd, NULL, NULL)) == -1) {
			printf("Error accept(): %s(%d)\n", strerror(errno), errno);
			continue;
		}
		
		//ե��socket����������
		p = 0;
		while (1) {
			int n = read(connfd, sentence + p, 8191 - p);
			if (n < 0) {
				printf("Error read(): %s(%d)\n", strerror(errno), errno);
				close(connfd);
				continue;
			} else if (n == 0) {
				break;
			} else {
				p += n;
				if (sentence[p - 1] == '\n') {
					break;
				}
			}
		}
		//socket���յ����ַ������������'\0'
		sentence[p - 1] = '\0';
		len = p - 1;
		
		//�ַ�������
		for (p = 0; p < len; p++) {
			sentence[p] = toupper(sentence[p]);
		}

		//�����ַ�����socket
 		p = 0;
		while (p < len) {
			int n = write(connfd, sentence + p, len + 1 - p);
			if (n < 0) {
				printf("Error write(): %s(%d)\n", strerror(errno), errno);
				return 1;
	 		} else {
				p += n;
			}			
		}

		close(connfd);
	}

	close(listenfd);
}

