# <center>基于TCP协议的网络聊天室</center>
###### <center>171180503 周晨晨  171180506 李昕宸 171180629 陈宇星 </center>  

## 1 服务器端

### 1.1 简介

&emsp;&emsp;为实现多人实时在线聊天的功能，所有的用户需要连接到一个共同的服务器以发送信息或获取他人信息，服务器在其中起到信息集中于管理的作用。  

### 1.2 用户数据库

每一个用户使用前需要注册聊天室账号，登陆后方可进入到聊天室中，未注册用户初次登陆需要首先注册账号。所有用户的账号与密码信息以字符串形式保存至服务器端的 log_in.ini配置文件中，当服务器程序运行时将从配置文件中读取所有的用户信息及密码，并存储到程序内建的struct结构组中构成用户数据库，每当收到用户登录请求时直接比对数据库判断账号与密码是否匹配，匹配成功则允许登录到聊天室，否则拒绝其登录请求。  

代码如下：

```c
//server.c
struct users_info
{
	char name[32];
	char pswd[64];
	int log_status;
};

//void main()
users_info* USERS;
FILE* fp;
if((fp=open("log_in.ini","r"))==NULL)
	{
		exit(-1);
	}
	int count;
	fscanf(fp,"%d",&count);
	USERS=(users_info*)malloc(count*sizeof(users_info));
	for(int k=0;k<count;k++)
	{
		char name[32],pswd[64];
		fscanf(fp,"%s\n",name);
		fscanf(fp,"%s\n",pswd);
		strcpy(USERS[k].name,name);
		strcpy(USERS[k].pswd,pswd);
		USERS[k].log_status=0;
	}
	fclose(fp);
```

### 1.3 TCP监听

聊天程序使用TCP协议，服务器在完成数据库加载及初始化操作后，随即进入主程序循环监听状态，等待接收来自用户的连接请求，并做出响应。  
用户端调用函数：
```c
connect(sockfd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr))
```
向服务器发出连接请求，服务器端运行等待监听函数：

```c
accept(listenfd,(struct sockaddr *)(&client_addr),&sin_size))
```
> 当无连接请求时，函数 accept 返回-1，当有连接请求时，返回所接收到socket套接字的描述值。
>
服务器端给出响应，服务器与用户程序完成TCP”握手“，建立初次连接。

### 1.4 登录请求处理

当用户程序与服务器端建立TCP连接后，用户端随即分三次向服务器端发送注册/登录、账号、密码信息。
为确保可靠性，服务器端每接收到一个信息，随即向客户端发送”OK“以示收到，客户端收到”OK“后再发送下一个信息，直至完成登录信息的发送。

```c
//server.c
memset(buffer,0,SIZE);
recv(connfd[i],buffer,SIZE,0);
strcpy(construction,buffer);
strcpy(buffer,"OK");
send(connfd[i],buffer,SIZE,0);
		
memset(buffer,0,SIZE);
recv(connfd[i],buffer,SIZE,0);
strcpy(name,buffer);
strcpy(buffer,"OK");
send(connfd[i],buffer,SIZE,0);

memset(buffer,0,SIZE);
recv(connfd[i],buffer,SIZE,0);
strcpy(pswd,buffer);
```
完成登录信息传递后，服务器端随即按照登录或注册申请分别执行检测与重建，若用户账号或密码输入错误，分别返回不同的错误代码到用户端，用户端再解析为不同的个错误提示信息，具体错误信息如下：

返回代码 | 提示信息 | 相应操作
:---:|:---:|:---:
OK | 登陆成功 | 允许登录
OK1 | 注册成功，自动登录 | 允许登录
error1 | 密码错误 | 拒绝登录请求
error2 | 用户账号错误,未注册 | 拒绝登录请求
error3 | 用户已登录 | 拒绝登录请求
error4 | 用户名重复 | 拒绝注册请求
error5 | 最大线程数（用户数）已满 | 拒绝所有请求

具体登录信息处理代码如下：
```c
//server.c
memset(buffer,0,SIZE);
int judge=0;
if(strcmp(construction,"log_in")==0)
{
	for(int k=0;k<count;k++)
	{
		if(strcmp(name,USERS[k].name)==0)
		{
			judge=1;
			if(strcmp(pswd,USERS[k].pswd)==0)
			{
				judge=2;
				strcpy(buffer,"OK");//登录成功
				break;
			}
			strcpy(buffer,"error1");//密码错误
		}
	}
	if(judge==0)
	{
		strcpy(buffer,"error2");//无此用户信息，未注册
	}
}
else
{
	judge = 2;
	memset(buffer, 0, SIZE);
	for (int k = 0; k < count; k++)
	{
		if (strcmp(name, USERS[k].name) == 0)
		{
			judge = 0;
			strcpy(buffer, "error4");
		}
	}
	if (judge != 0)
	{
		strcpy(USERS[count].name, name);
		strcpy(USERS[count].pswd, pswd);
		USERS[count].log_status = 0;
		count++;
		fp = fopen("log_in.ini", "w");
		fprintf(fp, "%d\n", count);
		for (int k = 0; k < count; k++)
		{
			fprintf(fp, "%s\n", USERS[k].name);
			fprintf(fp, "%s\n", USERS[k].pswd);
		}
		fclose(fp);
		strcpy(buffer, "error3");
	}
}
send(connfd[i],buffer,SIZE,0);
if (judge != 2)
{
	connfd[i]=-1;
	continue;
}
memset(buffer,0,SIZE);
recv(connfd[i],buffer,SIZE,0);
if(strcmp(buffer,"OK")!=0)
	continue;
```

### 1.5 多线程处理

为应对多个用户同时登陆，单线程处理难以完成同时应对多个用户的收发信息操作。
因此，特引入多线程机制，为每一个登录的用户创建一个接收信息同时转发信息到其它用户的子线程，以处理随机产生的接收、发送操作。
多线程编程的头文件以及几个重要的函数为：
```c
//头文件
#include<pthread.h>
//进程生成函数
pthread_create(&thread_handle, NULL, pthread_handle, &socked_index);
//进程结束函数
pthread_exit(0);
```
服务器每完成对一个用户的登录信息处理并完成登录后，除向用户发送欢迎信息外，还会创建一个用户对应的专用子进程，用于接收该用户所绑定的套接字发送过来的信息，并同时转发到其它套接字绑定的用户端，实现消息同步。同时释放主线程资源，达到更快的响应，避免阻塞。  

线程创建函数如下：
```c
int socked_index = i;//这里避免线程还未创建完成，i的值可能会被while循环修改
//创建线程行读写操作
ret = pthread_create(&thread_handle, NULL, pthread_handle, &socked_index);
if(ret != 0)
{
    perror("Create pthread_handle fail!");
	exit(-1);
}
```

线程处理函数如下：
```c
void* pthread_handle(void * arg)
{
    int index,i;
    index = *(int *)arg;
    printf("in pthread_recv,index = %d,connfd = %d\n",index,connfd[index]);
    char buffer[SIZE];
    while(1)
    {
        //用于接收信息
        memset(buffer,0,SIZE);
        if((recv(connfd[index],buffer,SIZE,0)) <= 0)
        {
            close(connfd[index]);
            connfd[index]=-1;
            pthread_exit(0);
        }
        printf(" %s\n",buffer);
		fprintf(record_log,"%s\n",buffer);
        for(i = 0; i < LISTEN_MAX ; i++)
        {
            if(connfd[i] != -1)
            {
                if(send(connfd[i],buffer,strlen(buffer),0) == -1)
                {
                    perror("send");
                    pthread_exit(0);
                }
            }
        }
    }
}
```

### 1.6 Linux守护进程
为实现随时随地的通信，服务器端程序必须时刻运行。为解决Linux服务器上用户进程随shell窗口退出而被停止运行，我们需要将服务器端程序转为后台运行。
因此这里我们采用了将程序写入守护进程的方式，保证程序时刻运行。  
程序将子进程挂载到/目录下，避免了进程随shell退出而被杀掉。
创建守护进程程序如下：
```c
void mydaemon(int ischdir, int isclose, int argc, char** argv)
{
	// 调用setsid() 的不能是进程组组长，当前程序有可能是进程组组长
	pid_t pid = fork();

	// 非子进程则退出
	if (pid != 0)
		exit(-1);

	// 父进程退出，留下子进程

	// 创建一个新的会话期，从而脱离原有的会话期
	// 进程同时与控制终端脱离
	setsid();

	// 此时子进程成为新会话期的领导和新的进程组长
	// 但这样的身份可能会通过fcntl去获到终端
	pid = fork();

	// 非子进程则退出
	if (pid != 0)
		exit(-1);

	// 此时留下来的是孙子进程,再也不能获取终端

	// 通常来讲, 守护进程应该工作在一个系统永远不会删除的目录下
	if (ischdir == 0)
	{
		chdir("/");
	}

	// 关闭输入输出和错误流 (通过日志来查看状态)
	if (isclose == 0)
	{
		close(0);
		close(1);
		close(2);
	}

	//去掩码位
	umask((mode_t)0);//sys/stat.h
	main_main(argc, argv);
}
```

### 1.7 日志转存

程序中增设两个文件，分别是，用于保存用户登录信息及时间的log_in.log，用于保存聊天记录的record.log，两个文件分别使用"a+"追加写入的方式，需一定时间后手动清除。  
文件写入代码如下：
```c
FILE* record_log;
FILE* log_in_log;

//log_in.log
memset(buffer,0,SIZE);
//记录时间（服务器时间）
time(&timep);
p_curtime = localtime(&timep);
strftime(buffer, sizeof(buffer), "%Y/%m/%d %H:%M:%S ", p_curtime);
//记录登录/注册的用户名
strcat(buffer,"username:");
strcat(buffer,name);
//记录用户IP地址
strcat(buffer," from IP:");
strcat(buffer,inet_ntoa(client_addr.sin_addr));
//输出到文件
log_in_log=fopen("log_in.log","a+");
fprintf(log_in_log,"%s\n",buffer);
fclose(log_in_log);


//record.log
memset(buffer,0,SIZE);
if((recv(connfd[index],buffer,SIZE,0)) <= 0)
{
    close(connfd[index]);
    connfd[index]=-1;
    pthread_exit(0);
}
//日志转存到 record.log
record_log=fopen("record.log","a+");
printf(" %s\n",buffer);
fprintf(record_log,"%s\n",buffer);
fclose(record_log);
```

### 1.8 完整代码

```c
//server.c
#include<stdio.h>   
#include<stdlib.h>
#include<sys/types.h> 
#include<sys/stat.h>
#include<netinet/in.h>  
#include<sys/socket.h> 
#include<string.h>
#include<unistd.h>
#include<signal.h>
#include<sys/ipc.h>
#include<errno.h>
#include<sys/shm.h>
#include<time.h>
#include<pthread.h>
#include <arpa/inet.h>

#define PORT 1778
#define SIZE 1024
#define SIZE_SHMADD 2048
#define LISTEN_MAX 50

int listenfd;
int connfd[LISTEN_MAX];
FILE* record_log;
struct tm *p_curtime;
time_t timep;

typedef struct users_info
{
	char name[32];
	char pswd[64];
	int log_status;
} users_info;

//套接字描述符
int get_sockfd()
{
    struct sockaddr_in server_addr;
    if((listenfd=socket(AF_INET,SOCK_STREAM,0))==-1)
    {
        perror("socket");
        exit(-1);
    }
    printf("Socket successful!\n");
    //sockaddr结构 
    bzero(&server_addr,sizeof(struct sockaddr_in)); 
    server_addr.sin_family=AF_INET;                
    server_addr.sin_addr.s_addr=htonl(INADDR_ANY); 
    server_addr.sin_port=htons(PORT);  

    // 设置套接字选项避免地址使用错误，为了允许地址重用，我设置整型参数（on）为 1 （不然，可以设为 0 来禁止地址重用）
    int on=1;
    if((setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on)))<0)  
    {  
        perror("setsockopt failed");  
        exit(-1);  
    }

    //绑定服务器的ip和服务器端口号
    if(bind(listenfd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr))==-1)     
    {
        perror("bind");
        exit(-1);   
    }
    printf("Bind successful!\n");
    //设置允许连接的最大客户端数     
    if(listen(listenfd,LISTEN_MAX)==-1)     
    {
        perror("bind");
        exit(-1); 
    }   
    printf("Listening.....\n");
    return listenfd;
}

void* pthread_handle(void * arg)
{
    int index,i;
    index = *(int *)arg;
    printf("in pthread_recv,index = %d,connfd = %d\n",index,connfd[index]);
    char buffer[SIZE];
    while(1)
    {
        //用于接收信息
        memset(buffer,0,SIZE);
        if((recv(connfd[index],buffer,SIZE,0)) <= 0)
        {
			//printf("pthread%d exit\n",index);
            close(connfd[index]);
            connfd[index]=-1;
            pthread_exit(0);
        }
		
		record_log=fopen("record.log","a+");
        printf(" %s\n",buffer);
		fprintf(record_log,"%s\n",buffer);
		fclose(record_log);
		
        for(i = 0; i < LISTEN_MAX ; i++)
        {
            if(connfd[i] != -1)
            {
                if(send(connfd[i],buffer,strlen(buffer),0) == -1)
                {
                    perror("send");
                    pthread_exit(0);
                } 
            }
        }  

    }
}
void quit()  
{  
    char msg[10];
    int i = 0;
    while(1)
    {
        printf("please enter 'Q' to quit server!\n");
        scanf("%s",msg);  
        if(strcmp("Q",msg)==0)  
        {  
            printf("now close server\n");  
            close(listenfd); 
            for(i = 0; i < LISTEN_MAX ; i++)  
            {
                if(connfd[i] != -1)  
                {
                    close(connfd[i]); 
                }  
            }       
            exit(0);  
        }  
    }  
}

int main_main(int argc, char **argv)
{
    struct sockaddr_in client_addr;
    int sin_size;
    pid_t ppid,pid;
	FILE* fp;
	FILE* log_in_log;
	users_info* USERS;
	
	char construction[10];
	char name[32];
	char pswd[64];
	char* flag;
	
	memset(construction,0,10);
	memset(name,0,32);
	memset(pswd,0,64);

    int num = 0,i = 0,ret;
    //线程标识号
    pthread_t thread_server_close,thread_handle;
    //unsigned char buffer[SIZE];
    char buffer[SIZE];
    //创建套接字描述符
    int listenfd = get_sockfd();
    //记录空闲的客户端的套接字描述符（-1为空闲）
	int full_flag=0;//线程已满
	
	//读取用户注册信息
	if((fp=fopen("log_in.ini","r"))==NULL)
	{
		exit(-1);
	}
	int count;
	fscanf(fp,"%d\n",&count);
	USERS=(users_info*)malloc(LISTEN_MAX*sizeof(users_info));
	for(int k=0;k<count;k++)
	{
		char name[32],pswd[64];
		fscanf(fp,"%s\n",name);
		fscanf(fp,"%s\n",pswd);
		strcpy(USERS[k].name,name);
		strcpy(USERS[k].pswd,pswd);
		USERS[k].log_status=0;
	}
	fclose(fp);
	
	//密码及用户
    for(i = 0 ; i < LISTEN_MAX; i++)  
    {  
        connfd[i]=-1;
    }

    //创建一个线程，对服务器程序进行管理（关闭）
    ret = pthread_create(&thread_server_close,NULL,(void*)(&quit),NULL);
    if(ret != 0)
    {
        perror("Create pthread_handle fail!");
        exit(-1);
    }
    while(1)
    {
        for(i=0;i < LISTEN_MAX;i++)
        {
            printf("i == %d\n",i);
            if(connfd[i]==-1)//表示套接字容器空闲，可用
            {
                break;
            }
        }
		if(i==LISTEN_MAX)
			full_flag=1;
		else
			full_flag=0;
        printf("before accept i == %d\n",i);
        //服务器阻塞,直到客户程序建立连接
        sin_size=sizeof(struct sockaddr_in);
		int conn;
        if((conn=accept(listenfd,(struct sockaddr *)(&client_addr),&sin_size))==-1)         
        {
            perror("accept");
            //exit(-1);//要continue还是exit，再考虑
			continue;
        }
		connfd[i]=conn;
		
        printf("Accept successful!\n");
        printf("connect to client %d : %s:%d \n",num , inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
		
		//解析用户信息
		memset(buffer,0,SIZE);
		recv(connfd[i],buffer,SIZE,0);
		strcpy(construction,buffer);
		strcpy(buffer,"OK");
		send(connfd[i],buffer,SIZE,0);
		
		memset(buffer,0,SIZE);
		recv(connfd[i],buffer,SIZE,0);
		strcpy(name,buffer);
		strcpy(buffer,"OK");
		send(connfd[i],buffer,SIZE,0);
		//printf("name:%s\n",name);
		
		memset(buffer,0,SIZE);
		recv(connfd[i],buffer,SIZE,0);
		strcpy(pswd,buffer);
		//printf("pswd:%s\n",pswd);
		
		if(full_flag)
		{
			memset(buffer,0,SIZE);
			strcpy(buffer,"error5");
			send(conn,buffer,SIZE,0);
			continue;
		}
		
		memset(buffer,0,SIZE);
		int judge=0;
		if(strcmp(construction,"log_in")==0)
		{
			for(int k=0;k<count;k++)
			{
				if(strcmp(name,USERS[k].name)==0)
				{
					judge=1;
					if(strcmp(pswd,USERS[k].pswd)==0)
					{
						judge=2;
						if(USERS[i].log_status==0)
						{
							judge=3;
							break;
						}
					}
					
				}
			}
			if(judge==0)
				strcpy(buffer,"error2");//无此用户信息，未注册
			else if(judge==1)
				strcpy(buffer,"error1");//密码错误
			else if(judge==2)
				strcpy(buffer,"error3");//重复登陆
			else if(judge==3)
				strcpy(buffer,"OK");//登录成功
			else
				strcpy(buffer,"ERROR");//未知错误
		}
		else
		{
			judge = 3;
			memset(buffer, 0, SIZE);
			for (int k = 0; k < count; k++)
			{
				if (strcmp(name, USERS[k].name) == 0)
				{
					judge = 0;
					strcpy(buffer, "error4");
				}
			}
			if (judge != 0)
			{
				strcpy(USERS[count].name, name);
				strcpy(USERS[count].pswd, pswd);
				USERS[count].log_status = 1;
				count++;
				fp = fopen("log_in.ini", "w");
				fprintf(fp, "%d\n", count);
				for (int k = 0; k < count; k++)
				{
					fprintf(fp, "%s\n", USERS[k].name);
					fprintf(fp, "%s\n", USERS[k].pswd);
				}
				fclose(fp);
				strcpy(buffer, "OK1");
			}
		}
		send(connfd[i],buffer,SIZE,0);
		if (judge != 3)
		{
			connfd[i]=-1;
			continue;
		}
		memset(buffer,0,SIZE);
		recv(connfd[i],buffer,SIZE,0);
		if(strcmp(buffer,"OK")!=0)
			continue;
		
		
		memset(buffer,0,SIZE);
		time(&timep);
        p_curtime = localtime(&timep);
        strftime(buffer, sizeof(buffer), "%Y/%m/%d %H:%M:%S ", p_curtime);
		strcat(buffer,"username:");
		strcat(buffer,name);
		
		log_in_log=fopen("log_in.log","a+");
		fprintf(log_in_log,"%s\n",buffer);
		fclose(log_in_log);
		
        //把界面发送给客户端
        memset(buffer,0,SIZE);
        strcpy(buffer,"\n-------------------Welecom come to chat room----------------------\n");
		strcat(buffer,"\n--------------Please enter Q to quit the chat room----------------\n");
        send(connfd[i],buffer,SIZE,0);

        //将加入的新客户发送给所有在线的客户端/
        //printf("before recv\n");
        
        //printf("after recv\n");
		memset(buffer,0,SIZE);
		strcpy(buffer,name);
        strcat( buffer," 加入聊天室....");
		record_log=fopen("record.log","a+");
		fprintf(record_log,"%s\n",buffer);
		fclose(record_log);
        int j;
        for(j = 0; j < LISTEN_MAX; j++)
        {
            if(connfd[j] != -1)
            {
                printf("j == %d\n",j);
                send(connfd[j],buffer,strlen(buffer),0);
            }
        }
        int socked_index = i;//这里避免线程还未创建完成，i的值可能会被while循环修改
        //创建线程行读写操作
        ret = pthread_create(&thread_handle, NULL, pthread_handle, &socked_index);
		if(ret != 0)
		{
			perror("Create pthread_handle fail!");
			exit(-1);
		}
   }
   return 0;
}

void mydaemon(int ischdir, int isclose, int argc, char** argv)
{
	// 调用setsid() 的不能是进程组组长，当前程序有可能是进程组组长
	pid_t pid = fork();

	// 非子进程则退出
	if (pid != 0)
		exit(-1);

	// 父进程退出，留下子进程

	// 创建一个新的会话期，从而脱离原有的会话期
	// 进程同时与控制终端脱离
	setsid();

	// 此时子进程成为新会话期的领导和新的进程组长
	// 但这样的身份可能会通过fcntl去获到终端
	pid = fork();

	// 非子进程则退出
	if (pid != 0)
		exit(-1);

	// 此时留下来的是孙子进程,再也不能获取终端

	// 通常来讲, 守护进程应该工作在一个系统永远不会删除的目录下
	if (ischdir == 0)
	{
		chdir("/");
	}

	// 关闭输入输出和错误流 (通过日志来查看状态)
	if (isclose == 0)
	{
		close(0);
		close(1);
		close(2);
	}

	//去掩码位
	umask((mode_t)0);//sys/stat.h
	main_main(argc, argv);
}

int main(int argc, char* argv[])
{
	mydaemon(1, 1, argc, argv);

	while (1);

	exit(EXIT_SUCCESS);
}

```

## 2 用户端

### 2.1 完整代码
```c
#include<stdio.h>
#include<netinet/in.h>  
#include<sys/socket.h> 
#include<sys/types.h>
#include<string.h>
#include<stdlib.h>
#include<netdb.h>
#include<unistd.h>
#include<signal.h>
#include<errno.h>
#include<time.h>
#include<pthread.h>

#define SIZE 1024
#define SEND_SIZE 106
#define SERV_PORT        1778
char name[32];
char pswd[64];

void* pthread_recv(void * arg)
{
    char buffer[SIZE];
    int sockfd = *(int *)arg;
    while(1)
    {
        //用于接收信息
        memset(buffer,0,SIZE);
        if(sockfd > 0)
        {
            if((recv(sockfd,buffer,SIZE,0)) <= 0)
            {
                close(sockfd);
                exit(1);
            }
            printf("%s\n",buffer);
        }
    }

}
void* pthread_send(void * arg)
{
    //时间函数
    char buffer[SIZE],buf[SIZE];
    int sockfd = *(int *)arg;
    struct tm *p_curtime;
    time_t timep;

    while(1)
    {
        memset(buf,0,SIZE);
        fgets(buf,SIZE,stdin);//获取用户输入的信息
        memset(buffer,0,SIZE);
        time(&timep);
        p_curtime = localtime(&timep);
        strftime(buffer, sizeof(buffer), "%Y/%m/%d %H:%M:%S", p_curtime);
        /*输出时间和客户端的名字*/
        strcat(buffer," \n昵称 ->");
        strcat(buffer,name);
        strcat(buffer,":\n\t");

        /*对客户端程序进行管理*/
        if(strncmp("Q",buf,1)==0)
        {
            printf("该客户端下线...\n");
            strcat(buffer,"退出聊天室！");
            if((send(sockfd,buffer,SIZE,0)) <= 0)
            {
                perror("error send");
            }
            close(sockfd);
            sockfd = -1;
            exit(0);
        }
        else
        {
            strncat(buffer,buf,strlen(buf)-1);
            strcat(buffer,"\n");
            if((send(sockfd,buffer,SIZE,0)) <= 0)
            {
                 perror("send");
            }
        }
    }
}
int main(int argc, char **argv)
{
    pid_t pid;
    int sockfd,confd;
    char buffer[SIZE],buf[SIZE];
	char sen_buff[SEND_SIZE];
	char name_recv[100],pswd_recv[100];
	char construction[10];
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    struct hostent *host;
    short port;
	//0为已注册用户，直接登录
	//1为未注册用户，先行注册
	
    //线程标识号
    pthread_t thread_recv,thread_send;
    void *status;
    int ret;
	
	memset(name,0,32);
	memset(pswd,0,64);
    //四个参数
    if(argc!=4)
    {
        fprintf(stderr,"已注册用户请输入: \"%s+log_in+用户名+密码登录！\"\n",argv[0]);
		fprintf(stderr,"未注册用户请输入: \"%s+sign_in+用户名+密码登录！\"\n",argv[0]);
        exit(1);
    }
	host=gethostbyname("122.152.205.193");
	if(!(strcmp(argv[1],"log_in")==0 || strcmp(argv[1],"sign_in")==0))
	{
		perror("construction error");
        exit(-1);
	}
	stpcpy(construction,argv[1]);
    strcpy(name_recv,argv[2]);
	if(strlen(name_recv)>32)
	{
		perror("name nonconform");
        exit(-1);
	}
	else
		strcpy(name,name_recv);
	
    printf("用户名:%s\n",name);
	
	strcpy(pswd_recv,argv[3]);
	if(strlen(pswd_recv)>64)
	{
		perror("password nonconform");
        exit(-1);
	}
	strcpy(pswd,pswd_recv);
	
    /*客户程序开始建立 sockfd描述符 */
    if((sockfd=socket(AF_INET,SOCK_STREAM,0)) < 0) 
    {
        perror("socket");
        exit(-1);
    }
    printf("Socket successful!\n");

    /*客户程序填充服务端的资料 */
    bzero(&server_addr,sizeof(server_addr)); // 初始化,置0
    server_addr.sin_family=AF_INET;          // IPV4
    server_addr.sin_port=htons(SERV_PORT);  // (将本机器上的short数据转化为网络上的short数据)端口号
    server_addr.sin_addr=*((struct in_addr *)host->h_addr); // IP地址
    /* 客户程序发起连接请求 */
    if(connect(sockfd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr)) < 0) 
    {
        perror("connect");
        exit(-1); 
    }
    printf("Connect successful!\n");
	
    /*将用户信息发送到服务器端*/
	memset(buffer,0,SIZE);
	strcpy(buffer,construction);
	send(sockfd,buffer,SIZE,0);
	//printf("%s\n",buffer);
	memset(buffer,0,SIZE);
	recv(sockfd,buffer,SIZE,0);
	
	
	memset(buffer,0,SIZE);
	strcpy(buffer,name);
	send(sockfd,buffer,SIZE,0);
	//printf("%s\n",buffer);
	memset(buffer,0,SIZE);
	recv(sockfd,buffer,SIZE,0);
	
	
	memset(buffer,0,SIZE);
	strcpy(buffer,pswd);
    send(sockfd,buffer,SIZE,0);
	//printf("%s\n",buffer);
	
	
	//接收登录成功信息
	memset(buffer,0,SIZE);
	recv(sockfd,buffer,SIZE,0);
	if(strcmp(buffer,"error1")==0)
	{
		printf("密码错误，请重试！\n");
		exit(-1);
	}
	else if(strcmp(buffer,"error2")==0)
	{
		printf("用户未注册，请先注册！\n");
		exit(-1);
	}
	else if(strcmp(buffer,"erroe3")==0)
	{
		printf("用户已登录，请勿重复登陆！\n");
		exit(-1);
	}
	else if(strcmp(buffer,"error4")==0)
	{
		printf("账号已存在，请勿重复注册！\n");
		exit(-1);
	}
	else if(strcmp(buffer,"error5")==0)
	{
		printf("聊天室人数已达上限，请稍后重试！\n");
		exit(-1);
	}
	else
	{
		if(strcmp(buffer,"OK")==0)
		{
			printf("登录成功！\n");
		}
		else if(strcmp(buffer,"OK1")==0)
		{
			printf("注册成功！已自动登录\n");
		}
		else
		{
			printf("未知网络错误，请重试！\n");
			exit(-1);
		}
	}
	memset(buffer,0,SIZE);
	strcpy(buffer,"OK");
	send(sockfd,buffer,SIZE,0);
	
    //创建线程行读写操作/
    ret = pthread_create(&thread_recv, NULL, pthread_recv, &sockfd);//用于接收信息
    if(ret != 0)
    {
        perror("Create thread_recv fail!");
        exit(-1);
    }
    ret = pthread_create(&thread_send, NULL, pthread_send, &sockfd);//用于发送信息
    if(ret != 0)
    {
        perror("Create thread_send fail!");
        exit(-1);
    }
    printf("wait for thread_recv \n");
    pthread_join(thread_recv, &status);
    printf("wait for thread_send \n");
    pthread_join(thread_send, &status);
    printf("close sockfd \n");
    close(sockfd);
    return 0;    
}
```