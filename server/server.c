#include "common.h"
#include "server.h"
#include "socket_util.h"
#include "handles.h"
#include <pthread.h>



void* communication(void* session);
void initial(session_t * const session);
void server(int port)
{
	int i =0;
	for(i=0;i < sizeof(users)/sizeof(users[0]);i ++){
		char temp[100];
		snprintf(temp, 100, "djjowfy%d", i);
		stpcpy(users[i].name,temp);
		stpcpy(users[i].password,temp);
	}

    int socket = create_server_socket(port);
    struct sockaddr_in client_address;
    int len = sizeof(client_address);
	session_t *sess;
	session_t *sess_last = NULL;
	chdir("/etc/share/");
    while(1){
       int connection = accept(socket,(struct sockaddr*) &client_address,&len);
       if(connection == -1){
         continue;
       }
	   	sess = (session_t *)malloc(sizeof(session_t));
	   sess->pre = sess_last;
	   sess->next = NULL;
	   sess_last = sess;
	   initial(sess);
	   sess->self = (void *)sess;
	   	if(sess->pre != NULL){
		 ((session_t *)(sess->pre))->next = sess;
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
	memset(session->name,0,sizeof(session->name));
}
void* communication(void* session){
    char welcome[] = "220 weclome\r\n";
	session_t * sess = session;
    write(sess->ctrl_fd,welcome,strlen(welcome));
    handles(sess);
	if(sess->pre != NULL){
		((session_t *)sess->pre)->next = sess->next;
		if(sess->next != NULL){
			((session_t *)sess->next)->pre = sess->pre;
		}
	}
	free((session_t *)sess->self);
    return NULL;
}
