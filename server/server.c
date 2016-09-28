#include "common.h"
#include "server.h"
#include "socket_util.h"
#include "handles.h"
#include <pthread.h>



void* communication(void* session);
void server(int port)
{

	
    int socket = create_server_socket(port);
    struct sockaddr_in client_address;
    int len = sizeof(client_address);
	session_t sess = 
	{
			/* 控制连接 */
			-1, "", "", "",
			/* 数据连接 */
			 -1, -1,"",
			/* 状态 */
			0,0,"/"
	};
	chdir("/etc/share/");
    while(1){

       int connection = accept(socket,(struct sockaddr*) &client_address,&len);
       if(connection == -1){
         continue;
       }
       pthread_t pid;
	   sess.ctrl_fd = connection;
       pthread_create(&pid,NULL,communication,(void *)&sess);
    }
   close(socket);
}

void* communication(void* session){
    char welcome[] = "220 weclome\r\n";
    write(((session_t *)session)->ctrl_fd,welcome,strlen(welcome));

    handles((session_t *)session);
    return NULL;
}
