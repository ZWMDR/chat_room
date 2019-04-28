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
typedef struct i_index
{
	int init;
	int k;
} i_index;

users_info* USERS;
i_index* INDEX;
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
	int k=INDEX[index].k;
    printf("in pthread_recv,index = %d,connfd = %d\n",index,connfd[index]);
    char buffer[SIZE];
	char buff[SIZE];
    while(1)
    {
        //用于接收信息
        memset(buffer,0,SIZE);
		memset(buff,0,SIZE);
        if((recv(connfd[index],buff,SIZE,0)) <= 0)
        {
			USERS[i].log_status=0;
            close(connfd[index]);
            connfd[index]=-1;
            pthread_exit(0);
        }
		send(connfd[index],buff,SIZE,0);
		if(strcmp(buff,"EXIT")==0)
		{
			USERS[i].log_status=0;
            close(connfd[index]);
            connfd[index]=-1;
            pthread_exit(0);
		}
		if((recv(connfd[index],buff,SIZE,0)) <= 0)
        {
			//printf("pthread%d exit\n",index);
			USERS[i].log_status=0;
            close(connfd[index]);
            connfd[index]=-1;
            pthread_exit(0);
        }
		memset(buff,0,SIZE);
		recv(connfd[index],buff,SIZE,0);
		
		time(&timep);
        p_curtime = localtime(&timep);
        strftime(buffer, sizeof(buffer), "%Y/%m/%d %H:%M:%S\n\t", p_curtime);
		strcat(buffer,USERS[INDEX[index].k].name);
		strcat(buffer,":\n\t");
		strcat(buffer,buff);
		printf(" %s\n",buffer);
		
		record_log=fopen("record.log","a+");
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
	
	char construction[10];
	char name[32];
	char pswd[64];
	char* flag;
	
	memset(construction,0,10);
	memset(name,0,32);
	memset(pswd,0,64);

    int num = 0,i = 0,ret;
	int k=0;
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
	INDEX=(i_index*)malloc(LISTEN_MAX*sizeof(i_index));
	for(int m=0;m<LISTEN_MAX;m++)
	{
		INDEX[m].init==0;
		INDEX[m].k=0;
	}
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
			close(conn);
			continue;
		}
		else
			connfd[i]=conn;
		
		memset(buffer,0,SIZE);
		int judge=0;
		if(strcmp(construction,"log_in")==0)
		{
			for(k=0;k<count;k++)
			{
				if(strcmp(name,USERS[k].name)==0)
				{
					judge=1;
					if(strcmp(pswd,USERS[k].pswd)==0)
					{
						judge=2;
						if(USERS[k].log_status==0)
						{
							USERS[k].log_status=1;
							INDEX[i].init=1;
							INDEX[i].k=k;
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
			for (k = 0; k < count; k++)
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
				USERS[k].log_status=1;
				INDEX[i].init=1;
				INDEX[i].k=count;
				count++;
				fp = fopen("log_in.ini", "w");
				fprintf(fp, "%d\n", count);
				for (k = 0; k < count; k++)
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
		strcat(buffer," from IP:");
		strcat(buffer,inet_ntoa(client_addr.sin_addr));
		
		log_in_log=fopen("log_in.log","a+");
		fprintf(log_in_log,"%s\n",buffer);
		fclose(log_in_log);
		
        //把界面发送给客户端
        memset(buffer,0,SIZE);
        strcpy(buffer,"\n--------------------欢迎来到电子垃圾聊天室-----------------------\n");
		strcat(buffer,"\n----------------------输入“Q”退出聊天室--------------------------\n");
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
