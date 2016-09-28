#include "common.h"
#include "server.h"
#include "socket_util.h"
#include "handles.h"
#include <pthread.h>



void* communication(void* session);
void initial(session_t * const session);

void server(int port)
{

	
    int socket = create_server_socket(port);
    struct sockaddr_in client_address;
    int len = sizeof(client_address);
	session_t *sess;
	chdir("/etc/share/");
    while(1){
	   sess = (session_t *)malloc(sizeof(session_t));
	   initial(sess);
	   sess->self = (void *)sess;
       int connection = accept(socket,(struct sockaddr*) &client_address,&len);
       if(connection == -1){
         continue;
       }
       pthread_t pid;
	   sess->ctrl_fd = connection;
       pthread_create(&pid,NULL,communication,(void *)sess);
    }
   close(socket);
}

void initial(session_t * const session){
	session->ctrl_fd = -1;
	memset(session->cmd,0,sizeof(session->cmd));
	memset(session->cmdline,0,sizeof(session->cmdline));
	memset(session->arg,0,sizeof(session->arg));
	session->pasv_listen_fd = -1;
	session->data_fd = -1;
	memset(session->data_buff,0,sizeof(session->data_buff));
	session->is_login = 0;
	session->type = 0;
	memset(session->work_path,0,sizeof(session->work_path));
	strcpy(session->work_path,"/etc/share/");
	session->self = NULL;
}
void* communication(void* session){
    char welcome[] = "220 weclome\r\n";
	session_t * sess = session;
    write(sess->ctrl_fd,welcome,strlen(welcome));
    handles(sess);
	free((session_t *)sess->self);
    return NULL;
}
