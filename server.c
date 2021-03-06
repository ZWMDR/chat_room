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
#include<stdlib.h>
#include<arpa/inet.h>

#define PORT 1778
#define SIZE 1024
#define SIZE_SHMADD 2048
#define LISTEN_MAX 100

int listenfd;
int connfd[LISTEN_MAX];
int online_count;
FILE* record_log;
FILE* log_in_log;
struct tm *p_curtime;
time_t timep;

typedef struct users_info
{
	char name[32];
	char pswd[64];
	int log_status;
} users_info;

users_info* USERS;

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
    unsigned int index;
	int i,k;
	int log_out_flag=0;
    index = *(unsigned int *)arg;
	k=0x0000FFFF&index;//该用户在用户信息数据库中的序号
	index=(0xFFFF0000&index)>>16;//该用户的线程序号
    printf("in pthread_recv,index = %d,connfd = %d\n",index,connfd[index]);
    char buffer[SIZE];
	char buff[SIZE];
	char time_ch[SIZE];
    while(1)
    {
        //用于接收信息
        memset(buffer,0,SIZE);
        if((recv(connfd[index],buffer,SIZE,0)) <= 0)
        {
			USERS[k].log_status=0;
            online_count=0;
			for(int ss=0;ss<LISTEN_MAX;ss++)
				if(USERS[ss].log_status)
					online_count++;

			memset(time_ch,0,SIZE);
			sprintf(time_ch,"SYS_SIGNAL_ONLINE_COUNT:%03d ",online_count);
			memset(buff,0,SIZE);
			time(&timep);
			p_curtime = localtime(&timep);
			strftime(buff, sizeof(buffer), "%Y/%m/%d %H:%M:%S\n", p_curtime);
			strcat(buff,"系统消息:\n\t");
			strcat(buff,USERS[k].name);
			strcat(buff,"已退出聊天室");
			printf("%s\n",buff);
			strcat(time_ch,buff);
			
			close(connfd[i]);
			connfd[index]=-1;
			log_out_flag=1;
        }
		else if(strncmp(buffer,"SYS_SIGNAL_IMG:",14)==0)//图片接收
		{
			printf("%s\n","Switch to img mode");
			char *p;
			char split[10][100]={0};
			const char *delim=":";
			int split_count=0;
			long count;
			char img_name[SIZE];
			p=strtok(buffer,delim);
			while(p)
			{
				strcpy(split[split_count++],p);
				p=strtok(NULL,delim);
			}
			memset(img_name,0,SIZE);
			memset(buff,0,SIZE);
			time(&timep);
			p_curtime = localtime(&timep);
			strftime(buff, sizeof(buff), "%Y_%m_%d_%H_%M_%S_", p_curtime);
			memset(buffer,0,SIZE);
			strcpy(img_name,"recv_imgs/");
			strcat(img_name,buff);
			strcat(img_name,split[1]);
			printf("img_name:%s\n",img_name);
			FILE *img=fopen(img_name,"wb");
			count=atol(split[2]);
			while(count>0)
			{
				memset(buffer,0,SIZE);
				long recv_len=recv(connfd[index],buffer,SIZE,0);
				fwrite(buffer,sizeof(char),recv_len,img);
				count-=recv_len;
			}
			fclose(img);
			
			//向用户转发图片
			struct stat statbuf;
			stat(img_name,&statbuf);
			count=statbuf.st_size;
			
			online_count=0;
			for(int ss=0;ss<LISTEN_MAX;ss++)
				if(USERS[ss].log_status)
					online_count++;

			memset(time_ch,0,SIZE);
			sprintf(time_ch,"SYS_SIGNAL_IMG:%s:%ld:",img_name,count);
			
			memset(buff,0,SIZE);
			time(&timep);
			p_curtime = localtime(&timep);
			strftime(buff, sizeof(buff), "%Y/%m/%d %H:%M:%S\n", p_curtime);
			strcat(buff,USERS[k].name);
			strcat(buff,":\n\t");
			strcat(buff,"发送了一张图片,已保存到程序目录下recv_imgs文件夹下");
			strcat(time_ch,buff);
			
			for(i = 0; i < LISTEN_MAX ; i++)
			{
				if(connfd[i] != -1)
				{
					if(send(connfd[i],time_ch,SIZE,0) == -1)
					{
						connfd[i]=-1;
					}
					if(connfd[i] != -1)
					{
						long send_count=count;
						img=fopen(img_name,"rb");
						while(send_count>0)
						{
							memset(buffer,0,SIZE);
							fread(buffer,sizeof(char),SIZE,img);
							long send_len=send(connfd[i],buffer,SIZE,0);
							send_count-=send_len;
							//usleep(2000);
						}
						fclose(img);
					}
				}
			}
			record_log=fopen("record.log","a+");
			printf(" %s\n",buff);
			fprintf(record_log,"%s\n",buff);
			fclose(record_log);
			continue;
		}
		else if(strncmp(buffer,"SYS_SIGNAL_FILE:",15)==0)//文件接收
		{
			char *p;
			char split[10][100]={0};
			const char *delim=":";
			int split_count=0;
			long count;
			char file_name[SIZE];
			p=strtok(buffer,delim);
			while(p)
			{
				strcpy(split[split_count++],p);
				p=strtok(NULL,delim);
			}
			memset(file_name,0,SIZE);
			memset(buff,0,SIZE);
			time(&timep);
			p_curtime = localtime(&timep);
			strftime(buff, sizeof(buff), "%Y_%m_%d_%H_%M_%S_", p_curtime);
			memset(buffer,0,SIZE);
			strcpy(file_name,"recv_files/");
			strcat(file_name,buff);
			strcat(file_name,split[1]);
			printf("file_name:%s\n",file_name);
			FILE *file=fopen(file_name,"wb");
			count=atol(split[2]);
			while(count>0)
			{
				memset(buffer,0,SIZE);
				long recv_len=recv(connfd[index],buffer,SIZE,0);
				fwrite(buffer,sizeof(char),recv_len,file);
				count-=recv_len;
			}
			fclose(file);
			
			//向用户转发文件
			struct stat statbuf;
			stat(file_name,&statbuf);
			count=statbuf.st_size;
			
			online_count=0;
			for(int ss=0;ss<LISTEN_MAX;ss++)
				if(USERS[ss].log_status)
					online_count++;

			memset(time_ch,0,SIZE);
			sprintf(time_ch,"SYS_SIGNAL_FILE:%s:%ld:",file_name,count);
			
			memset(buff,0,SIZE);
			time(&timep);
			p_curtime = localtime(&timep);
			strftime(buff, sizeof(buff), "%Y/%m/%d %H:%M:%S\n", p_curtime);
			strcat(buff,USERS[k].name);
			strcat(buff,":\n\t");
			strcat(buff,"发送了一份文件,已保存到程序目录下recv_files文件夹下\n文件名：");
			strcat(buff,file_name);
			strcat(time_ch,buff);
			
			for(i = 0; i < LISTEN_MAX ; i++)
			{
				if(connfd[i] != -1)
				{
					if(send(connfd[i],time_ch,SIZE,0) == -1)
					{
						connfd[i]=-1;
					}
				}
			}
			record_log=fopen("record.log","a+");
			printf(" %s\n",buff);
			fprintf(record_log,"%s\n",buff);
			fclose(record_log);
			
			for(i = 0; i < LISTEN_MAX ; i++)
			{
				if(connfd[i] != -1)
				{
					long send_count=count;
					file=fopen(file_name,"rb");
					while(send_count>0)
					{
						memset(buffer,0,SIZE);
						fread(buffer,sizeof(char),SIZE,file);
						long send_len=send(connfd[i],buffer,SIZE,0);
						send_count-=send_len;
						//usleep(2000);
					}
					fclose(file);
				}
			}
			continue;
		}
		else if(strncmp(buffer,"SYS_SIGNAL_EMOTION:",18)==0)//表情
		{
			char *p;
			char split[5][100]={0};
			const char *delim=":";
			int split_count=0;
			long count;
			char file_name[SIZE];
			p=strtok(buffer,delim);
			while(p)
			{
				strcpy(split[split_count++],p);
				p=strtok(NULL,delim);
			}
			char emoji[100];
			strcpy(emoji,split[1]);
			
			memset(buff,0,SIZE);
			time(&timep);
			p_curtime = localtime(&timep);
			strftime(buff, sizeof(buff), "%Y/%m/%d %H:%M:%S\n", p_curtime);
			strcat(buff,USERS[k].name);
			strcat(buff,":\n\t");
			strcat(buff,"向您发送了一个表情，该客户端暂不支持查看");
			
			memset(time_ch,0,SIZE);
			sprintf(time_ch,"SYS_SIGNAL_EMOTION:%s:%s",emoji,buff);
		}
		else if(strcmp(buffer,"SYS_SIGNAL_QUIT")==0)//用户退出
		{
			USERS[k].log_status=0;
			online_count=0;
			for(int ss=0;ss<LISTEN_MAX;ss++)
				if(USERS[ss].log_status)
					online_count++;

			memset(time_ch,0,SIZE);
			sprintf(time_ch,"SYS_SIGNAL_ONLINE_COUNT:%03d ",online_count);
			memset(buff,0,SIZE);
			time(&timep);
			p_curtime = localtime(&timep);
			strftime(buff, sizeof(buffer), "%Y/%m/%d %H:%M:%S\n", p_curtime);
			strcat(buff,"系统消息:\n\t");
			strcat(buff,USERS[k].name);
			strcat(buff,"已退出聊天室");
			printf("%s\n",buff);
			strcat(time_ch,buff);
			
			close(connfd[i]);
			connfd[index]=-1;
			log_out_flag=1;
			online_count--;
		}
		else//用户发送消息
		{
			online_count=0;
			for(int ss=0;ss<LISTEN_MAX;ss++)
				if(USERS[ss].log_status)
					online_count++;

			memset(time_ch,0,SIZE);
			sprintf(time_ch,"SYS_SIGNAL_ONLINE_COUNT:%03d ",online_count);
			
			memset(buff,0,SIZE);
			time(&timep);
			p_curtime = localtime(&timep);
			strftime(buff, sizeof(buff), "%Y/%m/%d %H:%M:%S\n", p_curtime);
			strcat(buff,USERS[k].name);
			strcat(buff,":\n\t");
			strcat(buff,buffer);
			strcat(time_ch,buff);
		}
		
		record_log=fopen("record.log","a+");
        printf(" %s\n",buff);
		fprintf(record_log,"%s\n",buff);
		fclose(record_log);
		
        for(i = 0; i < LISTEN_MAX ; i++)
        {
            if(connfd[i] != -1)
            {
                if(send(connfd[i],time_ch,SIZE,0) == -1)
                {
                    connfd[i]=-1;
                }
            }
        }
		if(log_out_flag)
		{
			pthread_exit(0);
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
//int main(int argc, char **argv)
{
    struct sockaddr_in client_addr;
    int sin_size;
    pid_t ppid,pid;
	FILE* fp;
	
	char construction[10];
	char name[32];
	char pswd[64];
	char* flag;
	
	USERS=(users_info*)malloc(LISTEN_MAX*sizeof(users_info));
	
	memset(construction,0,10);
	memset(name,0,32);
	memset(pswd,0,64);

	online_count=0;
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
		int user_id;
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
            //exit(-1);
			continue;
        }
		
        printf("Accept successful!\n");
        printf("connect to client %d : %s:%d \n",num , inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
		
		//解析用户信息
		memset(buffer,0,SIZE);
		recv(conn,buffer,SIZE,0);
		strcpy(construction,buffer);
		memset(buffer,0,SIZE);
		strcpy(buffer,"OK");
		send(conn,buffer,SIZE,0);
		
		memset(buffer,0,SIZE);
		recv(conn,buffer,SIZE,0);
		strcpy(name,buffer);
		memset(buffer,0,SIZE);
		strcpy(buffer,"OK");
		send(conn,buffer,SIZE,0);
		printf("\nname:%s\n",name);
		
		memset(buffer,0,SIZE);
		recv(conn,buffer,SIZE,0);
		strcpy(pswd,buffer);
		printf("pswd:%s\n",pswd);
		
		if(full_flag)
		{
			memset(buffer,0,SIZE);
			strcpy(buffer,"error5");
			send(conn,buffer,SIZE,0);
			continue;
		}
		connfd[i]=conn;
		
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
						if(USERS[k].log_status==0)
						{
							USERS[k].log_status=1;
							user_id=k;
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
		else//注册
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
				user_id=count;
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

		online_count++;
		
		memset(buffer,0,SIZE);
		time(&timep);
        p_curtime = localtime(&timep);
        strftime(buffer, sizeof(buffer), "%Y/%m/%d %H:%M:%S: ", p_curtime);
		strcat(buffer,"username:");
		strcat(buffer,name);
		strcat(buffer," from IP: ");
		strcat(buffer,inet_ntoa(client_addr.sin_addr));
		
		log_in_log=fopen("log_in.log","a+");
		fprintf(log_in_log,"%s\n",buffer);
		fclose(log_in_log);
		
        //把界面发送给客户端
		
		online_count=0;
		for(int ss=0;ss<LISTEN_MAX;ss++)
			if(USERS[ss].log_status)
				online_count++;
		
        memset(buffer,0,SIZE);
		sprintf(buffer,"SYS_SIGNAL_ONLINE_COUNT:%03d ",online_count);
        strcat(buffer,"\n------------------------欢迎来到电子垃圾聊天室----------------------------\n");
		strcat(buffer,"\n--------------------------输入“Q”退出聊天室-------------------------------\n");
        send(connfd[i],buffer,SIZE,0);
		
        //将加入的新客户发送给所有在线的客户端/
        memset(buffer,0,SIZE);
		sprintf(buffer,"SYS_SIGNAL_ONLINE_COUNT:%03d ",online_count);
		
		strcpy(buffer,name);
        strcat( buffer," 加入聊天室......\n");
		record_log=fopen("record.log","a+");
		fprintf(record_log,"%s\n",buffer);
		fclose(record_log);
		
		online_count++;
        int j;
        for(j = 0; j < LISTEN_MAX; j++)
        {
            if(connfd[j] != -1)
            {
                printf("j == %d\n",j);
                send(connfd[j],buffer,strlen(buffer),0);
            }
        }
		unsigned int socked_index=i;
		socked_index=socked_index<<16 | (unsigned int)user_id;
        //int socked_index = i;//这里避免线程还未创建完成，i的值可能会被while循环修改
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
