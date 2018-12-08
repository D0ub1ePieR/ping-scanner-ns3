#include"include_header.h"
#include"src_dst_ip.h"
#include"tcp_con_scan.h"

int main(int argc,char** argv)
{
	struct scansock scan; 
	int err,cmd;
	
	if ( argc > 1 )
	{
		printf("---don't give any param! just use %s\n", argv[0]);
		exit(1);
	}
	
	get_my_ip(&scan);
	get_dst(dst_ip, dst_name, &ipcount);
	scan.start_port=sport;
	scan.end_port=eport;
	printf("-------------------------\n");
	printf("*source IP is %s\n",scan.src_ip);
	//printf("*target IP is %s\n",scan.dst_ip);
	
	int i;
	for (i=0;i<ipcount;i++)
	{
		printf("*target host IP is %s\n",dst_ip[i]);
		printf("*target host name is %s\n",dst_name[i]);
		
		strcpy(scan.dst_ip,dst_ip[i]);
		
		pthread_t pidth;
		err=pthread_create(&pidth,NULL,tcp_con_scan,(void*)&scan);
		if (err != 0)
		{
			printf("pthread_create:%s\n",strerror(err));
			exit(0);
		}
		err=pthread_join(pidth,NULL);
		if (err != 0)
		{
			printf("pthread_join:%s\n",strerror(err));
			exit(0);
		}
	}
	printf("alive host ip-name:\n");
	for (i=0;i<ipcount;i++)
		printf("--%s(%s)\n",dst_name[i],dst_ip[i]);
	return 0;
}