#include "client.h"
#define SERVERPORT 8765
int main(int argc,char const *argv[])
{
    printf("please input server ip address:\n");
    char ip_address[50];
    gets(ip_address);
    client(ip_address,SERVERPORT);
    return 0;
}
