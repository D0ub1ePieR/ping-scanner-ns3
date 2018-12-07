#include"include_header.h"
#include"src_dst_ip.h"

int main(int argc,char** argv)
{
	struct scansock scan; 
	int err,cmd;
	if ( argc != 4 )
	{
		printf("usage: %s target_ip start_port end_port\n",argv[0]);
		exit(1);
	}
	strcpy(scan.dst_ip,argv[1]);
	scan.start_port=atoi(argv[2]);
	scan.end_port=atoi(argv[3]);
	get_my_ip(scan.src_ip);
	return 0;
}