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

// 客户端最大连接数
#define CLIENT_MAX 50

// 服务器端口号
#define PORT 5566

// 缓冲区大小
#define BUF_SIZE 4096

// 重定义socket地址类型
typedef struct sockaddr* SP;

// 客户端结构体
typedef struct Client
{
	int sock;
	pthread_t tid;
	char name[20];
	struct sockaddr_in addr;
}Client;

// 定义50个存储客户端的结构变量
Client clients[50];

// 定义信号量用于限制客户端的数量
sem_t sem;

// 信号处理函数
void sigint(int num)
{
	for(int i=0; i<10; i++)
	{
		if(clients[i].sock)
		{
			pthread_cancel(clients[i].tid);
		}
	}
	debug("服务器退出！");
	exit(EXIT_SUCCESS);
}

void client_eixt(Client* client)
{
	sem_post(&sem);
	close(client->sock);
	//client->sock = 0;
}
void client_send(Client* client,char* buf)
{
	size_t len = strlen(buf)+1;
	for(int i=0; i<CLIENT_MAX; i++)
	{
		if(clients[i].sock && clients[i].sock != client->sock)
		{
 			send(clients[i].sock,buf,len,0);
		}
	}
}

void* run(void* arg)
{
	Client* client = arg;
	char buf[BUF_SIZE] = {};

	// 接收昵称
	int ret_size = recv(client->sock,client->name,20,0);
	if(0 >= ret_size)
	{
		client_eixt(client);
		return NULL;
	}

	// 通知其它客户端新人上线
	sprintf(buf,"!!!欢迎%s进入聊天室!!!",client->name);
	client_send(client,buf);
	for(;;)
	{	
		// 接收消息
		//strcat(buf,"msg from:");
		//strcat(buf,client->name);
		ret_size = recv(client->sock,buf,BUF_SIZE,0);
		if(0 >= ret_size || 0 == strcmp("quit",buf))
		{
			// 通知其它客户端退出
			sprintf(buf,"!!!%s退出聊天室!!!",client->name);
			client_send(client,buf);
			client_eixt(client);
			return NULL;
		}
		strcat(buf,"(msg from->)");
		strcat(buf,client->name);
		client_send(client,buf);
		debug(buf);
	}
}

int main(int argc,const char* argv[])
{
	signal(SIGINT,sigint);
	debug("注册信号处理函数成功!");

	sem_init(&sem,0,CLIENT_MAX);
	debug("初始化信号量成功!");

	int svr_sock = socket(AF_INET,SOCK_STREAM,0);
	if(0 > svr_sock)
	{
		error("socket");
	}
	debug("创建socket对象成功!");

	struct sockaddr_in svr_addr = {};
	svr_addr.sin_family = AF_INET;
	svr_addr.sin_port = htons(PORT);
	svr_addr.sin_addr.s_addr = INADDR_ANY;
	socklen_t addrlen = sizeof(svr_addr);
	debug("准备通信地址成功!");

	if(bind(svr_sock,(SP)&svr_addr,addrlen))
	{
		error("bind");
	}
	debug("绑定socket对象和通信地址成功!");


	if(listen(svr_sock,10))
	{
		error("listen");
	}
	debug("设置监听socket监听成功!");

	for(;;)
	{
		debug("等待客户端连接...");

		sem_wait(&sem);
		int index = 0;
		while(clients[index].sock)
		{
			index++;
		}

		clients[index].sock = accept(svr_sock,(SP)&clients[index].addr,&addrlen);
		if(0 > clients[index].sock)
		{
			kill(getpid(),SIGINT);
		}

		debug("有新的客户端连接,from ip:%s",inet_ntoa(clients[index].addr.sin_addr));
		pthread_create(&clients[index].tid,NULL,run,&clients[index]);
	}
}
