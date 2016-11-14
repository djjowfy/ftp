#include "common.h"
#include "client.h"

#define SERVERPORT 8765
int main(int argc,char const *argv[])
{
	if(argc != 2){
		printf("please input server ip address:\n");
		return 0;
	}
    client(argv[1],SERVERPORT);
    return 0;
}
