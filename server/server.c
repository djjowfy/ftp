#include "common.h"
#include "server.h"
#include "socket_util.h"
#include "handles.h"
#include <pthread.h>
void* communication(void *connection_arg);
void server(int port)
{
    int socket = create_server_socket(port);
    struct sockaddr_in client_address;
    int len = sizeof(client_address);
    while(1){
       int *connection = (int*)malloc(sizeof(int));
       *connection = accept(socket,(struct sockaddr*) &client_address,&len);
       if((*connection) == -1){
         continue;
       }
       pthread_t pid;
       pthread_create(&pid,NULL,communication,(void *)connection);
    }
   close(socket);
}

void* communication(void* connection_arg){
    char welcome[] = "220 weclome\r\n";
    int connection = *(int *)connection_arg;
    write(connection,welcome,strlen(welcome));
    handles(connection);
    return NULL;
}
