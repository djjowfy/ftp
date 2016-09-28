#include "common.h"
#include "client.h"
#include "socket_util.h"
#include "handles.h"
#include <pthread.h>
void* communication(void *connection_arg);
void client(const char* ip_address,int port)
{
    const int socket = create_client_socket(ip_address,port);
    handles(socket);
    close(socket);
}

