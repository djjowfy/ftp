#include "common.h"
#include "client.h"
#include "socket_util.h"
#include "handles.h"
#include <pthread.h>
void* communication(void *connection_arg);
void initial(session_t * const session);
void client(const char* ip_address,int port)
{
	
	session_t *sess = (session_t *)malloc(sizeof(session_t));
	initial(sess);
    const int socket = create_client_socket(ip_address,port);
	sess->ctrl_fd = socket;
    handles(sess);
    close(sess->ctrl_fd);
	free((session_t *)sess->self);
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
	session->pre = NULL;
	session->next = NULL;
	session->alive = 1;
	session->modifier = 0;
	memset(session->ioline, 0, sizeof(session->ioline));
	memset(session->io_cmd, 0, sizeof(session->io_cmd));
	memset(session->io_arg, 0, sizeof(session->io_arg));
}
