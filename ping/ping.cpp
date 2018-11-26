#include<stdlib.h>
#include<stdio.h>
#include<ctype.h>
#include<sys/socket.h>	//引入recvfrom、sendto等函数
#include<sys/time.h>	//获取系统时间
#include<sys/types.h>	//数据类型定义
#include<time.h>
#include<string.h>
#include<errno.h>	//抛出错误信息
#include<unistd.h>
#include<arpa/inet.h>	//提供IP地址转换函数
#include<linux/tcp.h>	//提供通用的文件、目录、程序及进程操作的函数
#include<fcntl.h>	//提供对文件控制的函数
#include<netdb.h>	//提供设置及获取域名的函数
#include<signal.h>	//进行信号处理
#include<netinet/in.h>	//获取protocol参数的常值
#include<netinet/ip.h>	//ip帧头定义
#include<netinet/ip_icmp.h>	//icmp帧头定义
#include<pthread.h>		//提供多线程操作的函数

#define maxbuf 20480

//global
int pingflag;
int seq=0;		//报文序列号
int rev=0;
float min=10,max=0,total=0;

void alarm_timer(int signal)	//设置线程超时时长检测
{
	pingflag=-1;
	alarm(0);
}

unsigned short checksum(unsigned char* buf, unsigned int len)//求报文校验和，对每16位进行反码求和（高位溢出位会加到低位），即先对每16位求和，在将得到的和转为反码
{
	unsigned long sum = 0;   
	unsigned short *pbuf;  
    pbuf = (unsigned short*)buf;//转化成指向16位的指针  
    while(len > 1)//求和  
    {  
        sum+=*pbuf++;  
        len-=2;  
   	}  
    if(len)//如果len为奇数，则最后剩一位要求和  
        sum += *(unsigned char*)pbuf;  
    sum = (sum>>16)+(sum & 0xffff);//  
    sum += (sum>>16);//上一步可能产生溢出  
    return (unsigned short)(~sum);
}

int ping_target_by_send_icmp(char *dst_ip, int packet_length)	//通过构造icmp报文ping目标主机，检测是否存活
{
	unsigned char send_buf[maxbuf];
	unsigned char recv_buf[maxbuf];		//发送报文、接收报文
	struct sockaddr_in dst_addr;		//目标地址socket类
	int sockfd;

	if ( (sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0 )
	{
		perror("socket_icmp");
		exit(0);
	}		//判断是否成功建立socket

	memset(&dst_addr,0,sizeof(dst_addr));
	dst_addr.sin_family=AF_INET;	//设置协议类型为ipv4套接字
	if ( inet_pton(AF_INET, dst_ip, &(dst_addr.sin_addr)) != 1 )
	{
		perror("inet_pton:");
		exit(0);
	}		//将ip地址从十进制转换为二进制，并检测是否转换成功
	
	struct sigaction newthread,oldthread;
	newthread.sa_handler=alarm_timer;
	sigemptyset(&newthread.sa_mask);
	newthread.sa_flags=0;
	sigaction(SIGALRM,&newthread,&oldthread);
	alarm(30);
	
	struct icmp *t_icmp;
	struct ip *t_ip;		//icmp以及ip报文
	struct timeval *tvstart,*tvend;
	pid_t pid=getpid();		//获取进程号
	pingflag=0;		//判断目标主机是否存活
	while(pingflag==0)
	{
		memset(send_buf,0,maxbuf);
		//构造icmp报文
		t_icmp=(struct icmp*)send_buf;
		t_icmp->icmp_type = ICMP_ECHO;  
        t_icmp->icmp_code = 0;  
        t_icmp->icmp_cksum = 0;  
        t_icmp->icmp_id = pid;  
        t_icmp->icmp_seq = seq++; 
		//memcpy(t_icmp->icmp_data,data,sizeof(data));
        tvstart = (struct timeval*)t_icmp->icmp_data;  
        gettimeofday(tvstart, NULL); 
		t_icmp->icmp_cksum=checksum( (unsigned char *)t_icmp, packet_length + 8);
		
		sendto(sockfd, send_buf, packet_length + 8, 0, (struct sockaddr*)&dst_addr, sizeof(dst_addr));	//发送报文
		
		unsigned int len_ip,len_icmp;
		unsigned short sum_recv,sum_cal;
		double deltsec;
		char src_ip[INET_ADDRSTRLEN];
		int n;
		memset(recv_buf,0,maxbuf);
		n = recvfrom(sockfd, recv_buf, maxbuf, 0, NULL, NULL);		//接收回复报文
		if (n<0)
		{
			if (errno==EINTR)
				continue;
			else
			{
				perror("recvfrom");
				exit(0);
			}	
		}
		if( (tvend = (struct timeval*)malloc(sizeof(struct timeval))) < 0)  
        {  
            perror("malloc tvend:");  
            exit(0);  
        } 
		gettimeofday(tvend, NULL);
		t_ip=(struct ip*)recv_buf;
		len_ip=(t_ip->ip_hl)*4;		//ip报文头部长度
		len_icmp=ntohs(t_ip->ip_len)-len_ip;		//icmp报文长度
		t_icmp=(struct icmp*)( (unsigned char*)t_ip+len_ip );//必须强制转换
		sum_recv=t_icmp->icmp_cksum;	//记录收到的校验和
		t_icmp->icmp_cksum=0;
		sum_cal=checksum((unsigned char*)t_icmp, len_icmp);
		if(sum_cal != sum_recv)  
			//报文校验和出错
            printf("---checksum error\tsum_recv = %d\tsum_cal = %d\n",sum_recv, sum_cal);  
        else  
        {  
            switch(t_icmp->icmp_type)  
            {  
                case ICMP_ECHOREPLY:
					//接收到回复报文
                    pid_t pid_now, pid_rev;  
                    pid_rev = t_icmp->icmp_id;  
                    pid_now = getpid();  
                    if(pid_rev != pid_now )  
					//报文确认号不符
                    	printf("---pid not match! pin_now = %d, pin_rev = %d\n", pid_now, pid_rev);  
                    else  
                        pingflag = 1;  
                    inet_ntop(AF_INET, (void*)&(t_ip->ip_src), src_ip, INET_ADDRSTRLEN);  
                    //tvstart = (struct timeval*)t_icmp->icmp_data;  
                    deltsec = (tvend->tv_sec - tvstart->tv_sec) + (tvend->tv_usec - tvstart->tv_usec)/1000000.0;  
					if (deltsec*1000<min)	min=deltsec*1000;
					if (deltsec*1000>max)	max=deltsec*1000;
					total+=deltsec*1000;
					rev++;
                    printf("---%d bytes from %s: icmp_req=%d ttl=%d time=%4.2f ms\n", len_icmp, src_ip, t_icmp->icmp_seq, t_ip ->ip_ttl, deltsec*1000);//想用整型打印的话必须强制转换！  
                    break;  
               	case ICMP_TIME_EXCEEDED: 
					//响应超时
                    printf("---time out!\n");  
                    pingflag = -1;  
                    break;  
                case ICMP_DEST_UNREACH: 
					//无法连接到目标主机
                    inet_ntop(AF_INET, (void*)&(t_ip->ip_src), src_ip, INET_ADDRSTRLEN);  
                    printf("---From %s icmp_seq=%d Destination Host Unreachable\n", src_ip, t_icmp->icmp_seq);  
                    pingflag = -1;  
                    break;  
                default:   
					//其他接受错误
                    printf("recv error!\n");  
                    pingflag = -1;  
                    break;  
            }  
        }    	
	}
	alarm(0);
	sigaction(SIGALRM, &oldthread, NULL);
	return pingflag;
}

void help()
{
	printf("--usage: ping [dst_ip] (-n count) (-l length)");
	exit(0);
}

int validnum(char *src)
{
	int i, len=strlen(src);
	for (i=0; i<len; i++)
		if (!isalnum(src[i]))
			return -2;
	return atoi(src);
}

int main(int argc, char** argv)
{
	int packet_count=-1, packet_length=56;		//ping次数以及icmp包长度
	int i=0, s=2;
	if (argc < 2)
	{
		help();
		exit(0);
	}
	for (i=2; i<argc; i++)
	{
		if (strcmp(argv[i],"-n")==0 && i+1<argc)
		{
			packet_count = validnum(argv[i+1]);
			s+=2;
		}
		if (strcmp(argv[i],"-l")==0 && i+1<argc)
		{
			packet_length = validnum(argv[i+1]);
			s+=2;
		}
	}
	if (s!=argc)
		help();
	if (packet_count==-2)
	{
		printf("-n count : you give an incorrect number\n");
		exit(0);
	}
	if (packet_length<0)
	{
		printf("-l length : you give an incorrect number(>=64)\n");
		exit(0);
	}
	printf("ping %s packetsize( (%d)%d bytes )\n", argv[1], packet_length, packet_length+28);
	i=0;
	while (1)
	{
		if (i++ >= packet_count && packet_count!=-1)
			break;
		ping_target_by_send_icmp(argv[1], packet_length);
		sleep(1);
	}
	printf("------------------------------------\n");
	printf("send packet %d , recieve packet %d \n", packet_count, rev);
	printf("time min %.3f , max %.3f , avg %.3f\n",min,max,total/rev);
	return 0;
}