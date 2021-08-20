#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef DEBUG
	#define debug(format,...) {}
#else
	#define debug(format,...) \
	{\
		fprintf(stdout,"%s:%d:%s ",__func__,__LINE__,__TIME__);\
		fprintf(stdout,format,##__VA_ARGS__);\
		fprintf(stdout,"\n");\
	}
#endif//DEBUG

#define error(format,...)\
{\
	fprintf(stdout,"%s:%d:%s ",__func__,__LINE__,__TIME__);\
	fprintf(stdout,format,##__VA_ARGS__);\
	fprintf(stdout,":%m\n");\
	exit(EXIT_FAILURE);\
}

#define BUF_SIZE 	4096
#define SERVER_PORT 5566
#define SERVER_IP 	"192.168.0.104 "
typedef struct sockaddr* SP;

void* run(void* arg)
{
	int cli_sock = *(int*)arg;
	char buf[BUF_SIZE] = {};
	for(;;)
	{
		int ret_size = recv(cli_sock,buf,BUF_SIZE,0);
		if(0 >= ret_size)
		{
			printf("服务器正在升级，请稍候登录！\n");
			exit(EXIT_SUCCESS);
		}
		printf("\n%s\n",buf);
		fflush(stdout);
	}
}


int main(int argc,const char* argv[])
{
	printf("服务器创建socket...\n");
	int cli_sock = socket(AF_INET,SOCK_STREAM,0);
	if(0 > cli_sock)
	{
		error("socket");
	}
	
	printf("准备地址...\n");
	struct sockaddr_in cli_addr	= {};
	cli_addr.sin_family = AF_INET;
	cli_addr.sin_port = htons(SERVER_PORT);
	cli_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
	socklen_t addrlen = sizeof(cli_addr);

	printf("绑定连接服务器...\n");
	if(connect(cli_sock,(SP)&cli_addr,addrlen))
	{
		printf("服务器正在升级，请稍候登录！\n");
		return EXIT_SUCCESS;
	}

	char buf[BUF_SIZE] = {};
	printf("请输入你的眤称：");
	gets(buf);
	send(cli_sock,buf,strlen(buf)+1,0);

	pthread_t tid;
	pthread_create(&tid,NULL,run,&cli_sock);

	for(;;)
	{
		printf(">>>");
		gets(buf);
		send(cli_sock,buf,strlen(buf)+1,0);
		if(0 == strcmp("quit",buf))
		{
			printf("退出聊天室！\n");
			return EXIT_SUCCESS;
		}
	}
}
