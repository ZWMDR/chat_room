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
#define SERV_PORT 1778
char name[32];
char pswd[64];

void* pthread_recv(void * arg)
{
    char buffer[SIZE];
	char buff[SIZE];
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
			if(strncmp(buffer,"SYS_SIGNAL_ONLINE_COUNT:",23)==0)
			{
				char* buf=buffer;
				printf("%s\n",buf+=28);
			}
			else if(strncmp(buffer,"SYS_SIGNAL_IMG:",14)==0)
			{
				int split_count=0;
				char *p;
				char split[10][100]={0};
				const char *delim=":";
				long count;
				char img_name[SIZE];
				p=strtok(buffer,delim);
				while(p)
				{
					strcpy(split[split_count++],p);
					//printf("%s\n",p);
					p=strtok(NULL,delim);
				}
				strcpy(img_name,split[1]);
				count=atol(split[2]);
				FILE *img=fopen(img_name,"wb");
				while(count>0)
				{
					memset(buffer,0,SIZE);
					long recv_len=recv(sockfd,buffer,SIZE,0);
					fwrite(buffer,sizeof(char),recv_len,img);
					count-=recv_len;
				}
				fclose(img);
				
				memset(buff,0,SIZE);
				for(int i=3;i<split_count;i++)
				{
					strcat(buff,split[i]);
					if(i<split_count-1)
						strcat(buff,":");
				}
				printf("%s\n",buff);
			}
			else if(strncmp(buffer,"SYS_SIGNAL_FILE:",15)==0)
			{
				int split_count=0;
				char *p;
				char split[10][100]={0};
				const char *delim=":";
				long count;
				char file_name[SIZE];
				p=strtok(buffer,delim);
				while(p)
				{
					strcpy(split[split_count++],p);
					//printf("%s\n",p);
					p=strtok(NULL,delim);
				}
				strcpy(file_name,split[1]);
				count=atol(split[2]);
				FILE *file=fopen(file_name,"wb");
				while(count>0)
				{
					memset(buffer,0,SIZE);
					long recv_len=recv(sockfd,buffer,SIZE,0);
					fwrite(buffer,sizeof(char),recv_len,file);
					count-=recv_len;
				}
				fclose(file);
				
				memset(buff,0,SIZE);
				for(int i=3;i<split_count;i++)
				{
					strcat(buff,split[i]);
					if(i<split_count-1)
						strcat(buff,":");
				}
				printf("%s\n",buff);
			}
			else if(strncmp(buffer,"SYS_SIGNAL_EMOTION:",18)==0)
			{
				int split_count=0;
				char *p;
				char split[10][100]={0};
				const char *delim=":";
				long count;
				char file_name[SIZE];
				p=strtok(buffer,delim);
				while(p)
				{
					strcpy(split[split_count++],p);
					//printf("%s\n",p);
					p=strtok(NULL,delim);
				}
				memset(buff,0,SIZE);
				for(int i=3;i<split_count;i++)
				{
					strcat(buff,split[i]);
					if(i<split_count-1)
						strcat(buff,":");
				}
				printf("%s\n",buff);
			}
			else
				printf("%s\n",buffer);
        }
    }

}
void* pthread_send(void * arg)
{
    //时间函数
    char buffer[SIZE],buf[SIZE];
    int sockfd = *(int *)arg;

    while(1)
    {
        memset(buf,0,SIZE);
        fgets(buf,SIZE,stdin);//获取用户输入的信息
        memset(buffer,0,SIZE);


        /*对客户端程序进行管理*/
        if(strncmp("Q",buf,1)==0)
        {
            printf("该客户端下线...\n");
            strcpy(buffer,"SYS_SIGNAL_QUIT");
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
			//printf("send");
            strncat(buffer,buf,strlen(buf)-1);
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