#include "common.h"
#include "handles.h"
#include "socket_util.h"
#include "file_util.h"
#include "str_utils.h"
#include "ftp_codes.h"
#include "list_dir.h"
#include <sys/select.h>
#include <sys/time.h>
#define STDIN 0

typedef struct ftpcmd
{
	const char *cmd;
	void (*cmd_handler)(session_t * const session);
} ftpcmd_t;

static void chatCmdHandler(session_t * const session);
static void userCmdHandler(session_t * const session);
static void passCmdHandler(session_t * const session);
static void quitCmdHandler(session_t * const session);
static void lsCmdHandler(session_t * const session);
static void getFileCmdHandler(session_t * const session);
static void sendFileCmdHandler(session_t * const session);
static void acceptResponse(session_t * const session);
static void clearIOBuffer(session_t * const session);
static void acceptIOBuffer(session_t * const session);
static void clearSessionCtrlData(session_t * const session);
static void sendCtrlResponse(session_t * const session,const char* cmd,const char* data);
static void get_ipaddr(session_t * const session);
static void getRemotePWD(session_t * const session);

static ftpcmd_t ctrl_cmds[] = {
	{"ABOR",NULL},//(ABORT)此命令使服务器终止前一个FTP服务命令以及任何相关数据传输。
	{"ACCT",NULL},//(ACCOUNT)此命令的参数部分使用一个Telnet字符串来指明用户的账户。
	{"ADAT",NULL},//(AUTHENTICATION/SECURITY DATA)认证/安全数据
	{"ALLO",NULL},//为接收一个文件分配足够的磁盘空间
	{"APPE",NULL},//增加
	{"AUTH",NULL},//认证/安全机制
	{"CCC",NULL},//清除命令通道
	{"CDUP",NULL},//改变到父目录
	{"CONF",NULL},//机密性保护命令
	{"CWD",NULL},//改变工作目录
	{"DELE",NULL},//删除文件
	{"ENC",NULL},//隐私保护通道
	{"EPRT",NULL},//为服务器指定要连接的扩展地址和端口
	{"EPSV",NULL},//进入扩展被动模式
	{"FEAT",NULL},//获得服务器支持的特性列表
	{"HELP",NULL},//如果指定了命令，返回命令使用文档；否则返回一个通用帮助文档
	{"LANG",NULL},//语言协商
	{"LIST",NULL},//如果指定了文件或目录，返回其信息；否则返回当前工作目录的信息
	{"LPRT",NULL},//为服务器指定要连接的长地址和端口
	{"LPSV",NULL},//进入长被动模式
	{"MDTM",NULL},//返回指定文件的最后修改时间
	{"MIC",NULL},//完整性保护命令
	{"MKD",NULL},//创建目录
	{"MLSD",NULL},//如果目录被命名，列出目录的内容
	{"MLST",NULL},//提供命令行指定的对象的数据
	{"MODE",NULL},//设定传输模式（流、块或压缩）
	{"NLST",NULL},//返回指定目录的文件名列表
	{"NOOP",NULL},//无操作（哑包；通常用来保活）
	{"OPTS",NULL},//为特性选择选项
	{"PASS",passCmdHandler},//认证密码
	{"PASV",NULL},//进入被动模式
	{"PBSZ",NULL},//保护缓冲大小
	{"PORT",NULL},//指定服务器要连接的地址和端口
	{"PROT",NULL},//数据通道保护级别
	{"PWD",NULL},//打印工作目录，返回主机的当前目录
	{"QUIT",quitCmdHandler},//断开连接
	{"REIN",NULL},//重新初始化连接
	{"REST",NULL},//从指定点重新开始传输
	{"RETR",NULL},//传输文件副本
	{"RMD",NULL},//删除目录
	{"RNFR",NULL},//从...重命名
	{"RNTO",NULL},//重命名到...
	{"SITE",NULL},//发送站点特殊命令到远端服务器
	{"SIZE",NULL},//返回文件大小
	{"SMNT",NULL},//挂载文件结构
	{"STAT",NULL},//返回当前状态
	{"STOR",NULL},//接收数据并且在服务器站点保存为文件
	{"STOU",NULL},//唯一地保存文件
	{"STRU",NULL},//设定文件传输结构
	{"SYST",NULL},//返回系统类型
	{"TYPE",NULL},//设定传输模式（ASCII/二进制).
	{"USER",userCmdHandler},//认证用户名
	{"XCUP",NULL},//改变之当前工作目录的父目录
	{"XMKD",NULL},//创建目录
	{"XPWD",NULL},//打印当前工作目录
	{"XRCP",NULL},//
	{"XRMD",NULL},//删除目录
	{"XRSQ",NULL},//
	{"XSEM",NULL},//发送，否则邮件
	{"XSEN",NULL},//发送到终端
	{"CHAT",chatCmdHandler},//聊天
	{"LS",lsCmdHandler},
	{"GET",getFileCmdHandler},
	{"PUT",sendFileCmdHandler}
};

volatile  int recv_data_flag = 0;


void* recv_ftp(void* session){
	session_t * sess = session;
    while(sess->alive){
		while(sess->modifier == 1);
		clearSessionCtrlData(sess);
		acceptResponse(sess);
		if(strcmp(sess->cmd,"666") == 0){
			recv_data_flag = 1;
			printf("\nrecv message: %s\n",sess->arg);
			fflush(stdin);
			fflush(stdout);
			clearIOBuffer(session);
			continue;
		}
		sess->modifier = 1;
	}
    return NULL;
}

static void getRemotePWD(session_t * const session){
	if(session->is_login){
		//while(session->modifier == 1);
		sendCtrlResponse(session,"PWD",NULL);
		clearSessionCtrlData(session);
		acceptResponse(session);
		if(strcmp(session->cmd,"257") == 0){
			printf("%s\n",session->arg);
		}else {
			printf("error\n");
		}
	}else{
		printf("please login first\n");

	}
}

static void get_ipaddr(session_t * const session){
  char *data;
  strtok_r(session->arg,"(",&data);
  char* ip[4];
  int i;
  for(i = 0;i < 4;i ++){
   ip[i] = strtok_r(data,",",&data);
  }
  char* port_1;
  port_1 = strtok_r(data,",",&data);
  char * port_2_temp;
  port_2_temp = strtok_r(data,",",&data);
  char port_2[4];
  memcpy(port_2,port_2_temp,strlen(port_2_temp) - 1);
  snprintf((session->data_ip),(sizeof(session->data_ip)),"%s%s%s%s%s%s%s",ip[0],".",ip[1],".",ip[2],".",ip[3]);
  session->data_port = atoi(port_1) * 256 + atoi(port_2);
  printf("server data ip is : %s:%d\n",session->data_ip,session->data_port);
}

static void sendPASV(session_t * const session){
	if(session->is_login){
		//while(session->modifier == 1);
		sendCtrlResponse(session,"PASV",NULL);
		clearSessionCtrlData(session);
		acceptResponse(session);
		if(strcmp(session->cmd,"227") == 0){
			get_ipaddr(session);
		}else {
			printf("error\n");
		}
	}else{
		printf("please login first\n");
	}
}



static void getFileCmdHandler(session_t * const session){
	if(session->is_login){
		getRemotePWD(session);
		char remote_file[1024];
		char local_pwd[1024];
		char local_file[1024];
		char temp_1[1024];
		char temp_2[1024];
		char *data;
		char *left;
		strSplit(&left,&data,session->io_arg," ",0);
		//left = strtok_r(session->io_arg," ",&data);
		int i = 0;
		for(i = 1; i < strlen(session->arg);i ++){
			session->arg[i - 1] = session->arg[i];
			if(session->arg[i] == '\"'){
				session->arg[i] = '\0';
			}
		}

		session->arg[strlen(session->arg) - 1] = '\0';
		if(data == NULL){
			getcwd(local_pwd,sizeof(local_pwd));
			snprintf(remote_file,sizeof(remote_file),"%s/%s",session->arg,session->io_arg);
			snprintf(local_file,sizeof(local_file),"%s/%s",local_pwd,session->io_arg);
		}else{
			while((*data) == ' ')data ++;
			snprintf(remote_file,sizeof(remote_file),"%s/%s",session->arg,left);
			if((*data) == '/'){
				if(*(data + strlen(data) - 1) == '/'){
					
					//printf("绝对路径，无文件名%c",*(data + strlen(data) - 1));
					snprintf(local_file,sizeof(local_file),"%s/%s",data,left);
				}else{
					//printf("绝对路径，有文件名%c",*(data + strlen(data) - 1));
					strcpy(local_file,data);
				}
			}else{
				getcwd(local_pwd,sizeof(local_pwd));
				if(*(data + strlen(data) - 1) == '/'){
					//printf("相对路径，无文件名%c",*(data + strlen(data) - 1));
					snprintf(local_file,sizeof(local_file),"%s/%s%s",local_pwd,data,left);
				}else{
					//printf("相对路径，有文件名%c",*(data + strlen(data) - 1));
					snprintf(local_file,sizeof(local_file),"%s/%s",local_pwd,data);
				}	
			}
		}
		FILE *fp = fopen(local_file ,"w");  
		if(NULL == fp)  
		{  
			printf("File:\t%s Can Not Open To Write\n",local_file );  
			return ;  
		}
		//get size
		sendCtrlResponse(session,"SIZE",remote_file);
		clearSessionCtrlData(session);
		acceptResponse(session);
		if(strcmp(session->cmd,"213") == 0){
			
		}else {
			printf("remote file error\n");
			return;
		}
		
		sendPASV(session);
		if(session->data_port == -1){
			printf("data socket error");
			return;
		}
		const int data_socket = create_client_socket(session->data_ip,session->data_port);
		sendCtrlResponse(session,"RETR",remote_file);
		clearSessionCtrlData(session);
		acceptResponse(session);
		if(strcmp(session->cmd,"150") == 0){
			char buffer[1024];
			bzero(buffer, sizeof(buffer));  
			int recv_count;  
			while((recv_count = recv(data_socket, buffer, sizeof(buffer),0)) > 0)  
			{  
				if(fwrite(buffer, sizeof(char), recv_count, fp) < recv_count)  
				{  
					printf("File:\t%s Write Failed\n", local_file);  
					fclose(fp);
					close(data_socket);
					return;  
				}  
				bzero(buffer, sizeof(buffer));  
			}
		}else {
			printf("error\n");
			fclose(fp);
			close(data_socket);
			return;
		}	
		fclose(fp);
		printf("Receive File:\t%s From Server IP Successful!\n", local_file);
		close(data_socket);
	}else{
		printf("please login first\n");
	}
	session->data_port = -1;
}

//put local remote
static void sendFileCmdHandler(session_t * const session){
	if(session->is_login){
		getRemotePWD(session);
		char remote_file[1024];
		char remote_pwd[1024];
		char local_pwd[1024];
		char local_file[1024];
		char temp_1[1024];
		char temp_2[1024];
		char *data;
		char *left;
		strSplit(&left,&data,session->io_arg," ",0);
		int i = 0;
		for(i = 1; i < strlen(session->arg);i ++){
			session->arg[i - 1] = session->arg[i];
			if(session->arg[i] == '\"'){
				session->arg[i] = '\0';
			}
		}

		session->arg[strlen(session->arg) - 1] = '\0';
		/**************************本地路径*****************************************/
		getcwd(local_pwd,sizeof(local_pwd));
		if((*left) == '/'){
			if(*(left + strlen(left) - 1) == '/'){
				printf("only can send regular file to server");
				return;
				//printf("绝对路径，无文件名%c",*(data + strlen(data) - 1));
				//snprintf(local_file,sizeof(remote_file),"%s/%s",local_pwd,left);
			}else{
				//printf("绝对路径，有文件名%c",*(data + strlen(data) - 1));
				//snprintf(local_file,sizeof(local_file),"%s/%s",local_pwd,left);
				strcpy(local_file,left);
			}
			}else{
				//getcwd(local_pwd,sizeof(local_pwd));
				if(*(left + strlen(left) - 1) == '/'){
					//printf("相对路径，无文件名%c",*(data + strlen(data) - 1));
					printf("only can send regular file to server");
					return;
					//snprintf(local_file,sizeof(local_file),"%s/%s%s",local_pwd,data,left);
				}else{
					//printf("相对路径，有文件名%c",*(data + strlen(data) - 1));
					snprintf(local_file,sizeof(local_file),"%s/%s",local_pwd,left);
				}	
		}
		/*******************远程路径*********************************************/	
		if(data == NULL){
			//无远程路径
			//getcwd(local_pwd,sizeof(local_pwd));
			snprintf(remote_file,sizeof(remote_file),"%s/%s",session->arg,session->io_arg);
		}else{
			while((*data) == ' ')data ++;
			//snprintf(remote_file,sizeof(remote_file),"%s/%s",session->arg,left);
			if((*data) == '/'){
				if(*(data + strlen(data) - 1) == '/'){
					
					//printf("绝对路径，无文件名%c",*(data + strlen(data) - 1));
					snprintf(remote_file,sizeof(remote_file),"%s/%s",data,left);
				}else{
					//printf("绝对路径，有文件名%c",*(data + strlen(data) - 1));
					strcpy(remote_file,data);
				}
			}else{
				getcwd(local_pwd,sizeof(local_pwd));
				if(*(data + strlen(data) - 1) == '/'){
					//printf("相对路径，无文件名%c",*(data + strlen(data) - 1));
					snprintf(remote_file,sizeof(remote_file),"%s/%s%s",session->arg,data,left);
				}else{
					//printf("相对路径，有文件名%c",*(data + strlen(data) - 1));
					snprintf(remote_file,sizeof(remote_file),"%s/%s",session->arg,data);
				}	
			}
		}
		FILE *fp = fopen(local_file, "r");  
		if(NULL == fp)  
		{  
			printf("File:\t%s Can Not Open To Write\n",local_file );  
			return ;  
		}

		
		sendPASV(session);
		if(session->data_port == -1){
			printf("data socket error");
			return;
		}
		const int data_socket = create_client_socket(session->data_ip,session->data_port);
		sendCtrlResponse(session,"STOR",remote_file);
		clearSessionCtrlData(session);
		acceptResponse(session);
		if(strcmp(session->cmd,"150") == 0){
			char buffer[1024];
			bzero(buffer, sizeof(buffer));   
            int length = 0;  
            while((length = fread(buffer, sizeof(char), sizeof(buffer), fp)) > 0)  
            {  
                if(send(data_socket, buffer, length, 0) < 0)  
                {  
                    printf("Send File:%s Failed./n", local_file);  
					fclose(fp);
					close(data_socket);
					return; 
                }  
                bzero(buffer, sizeof(buffer));  
            }  			
		}else {
			printf("error\n");
			fclose(fp);
			close(data_socket);
			return;
		}	
		fclose(fp);
		printf("Send File:\t%s To Server IP Successful!\n", local_file);
		close(data_socket);
	}else{
		printf("please login first\n");
	}
	session->data_port = -1;
}

static void lsCmdHandler(session_t * const session){
	if(session->is_login){
		getRemotePWD(session);
		sendPASV(session);
		if(session->data_port == -1){
			printf("data socket error");
			return;
		}
		const int data_socket = create_client_socket(session->data_ip,session->data_port);
		sendCtrlResponse(session,"LIST","-aL");
		clearSessionCtrlData(session);
		acceptResponse(session);
		if(strcmp(session->cmd,"150") == 0){
			char buffer[1024];
			bzero(buffer, sizeof(buffer));  
			int recv_count;  
			 while((recv_count = recv(data_socket, buffer, sizeof(buffer),0)) > 0)  
			{  
				printf("%s",buffer);
				bzero(buffer, sizeof(buffer));  
			}
		}else {
			printf("error\n");
		}	
		close(data_socket);
	}else{
		printf("please login first\n");
	}
	session->data_port = -1;
}

static void chatCmdHandler(session_t * const session){
	if(session->is_login){
		//while(session->modifier == 1);
		sendCtrlResponse(session,"CHAT",session->io_arg);
		clearSessionCtrlData(session);
		acceptResponse(session);
		if(strcmp(session->cmd,"200") == 0){
			printf("send success\n");
		}else {
			printf("error\n");
		}
		session->modifier = 0;
	}else{
		printf("please login first\n");
	}
}
static void userCmdHandler(session_t * const session){
	session->is_login = 0;
	memset(session->name,0,sizeof(session->name));
	sendCtrlResponse(session,"USER",session->io_arg);
	clearSessionCtrlData(session);
	acceptResponse(session);
	if(strcmp(session->cmd,"331") == 0){
		printf("username OK,please input password(format PASS your_password)\n");
		strcpy(session->name,session->io_arg);
	}else if(strcmp(session->cmd,"530") == 0){
		printf("username not exit,please re input username\n");
	}else{
		printf("error,please re input username\n");
	}
	session->modifier = 0;
}

static void passCmdHandler(session_t * const session){
	session->is_login = 0;
	// if(strlen(session->name) == 0){
		// printf("please input username\n");
		// return;
	// }
	sendCtrlResponse(session,"PASS",session->io_arg);
	clearSessionCtrlData(session);
	acceptResponse(session);
	if(strcmp(session->cmd,"230") == 0){
		printf("login success\n");
		session->is_login = 1;
		getRemotePWD(session);
	}else if(strcmp(session->cmd,"530") == 0){
		printf("password error,please re input password\n");
	}else{
		printf("error,please re input password\n");
	}
	session->modifier = 0;
}

static void quitCmdHandler(session_t * const session){
	session->alive = 0;
}

static void clearSessionCtrlData(session_t * const session){
	memset(session->cmdline, 0, sizeof(session->cmdline));
	memset(session->cmd, 0, sizeof(session->cmd));
	memset(session->arg, 0, sizeof(session->arg));
}

static void clearIOBuffer(session_t * const session){
	memset(session->ioline, 0, sizeof(session->ioline));
	memset(session->io_cmd, 0, sizeof(session->io_cmd));
	memset(session->io_arg, 0, sizeof(session->io_arg));
}
static void acceptIOBuffer(session_t * const session){
  size_t size = sizeof(session->ioline);
  char *buffer = session->ioline;
  int accept_count = getline(&buffer,&size,stdin);
  //printf("rcv count %d\n",accept_count);
  if(!(accept_count > 0)){
   return;
  }
  char * head = session->io_cmd;
  char * tail = session->io_arg;
  strSplit(&head,&tail,session->ioline," ",1);
  strUpper(session->io_cmd);
  strTrimCrlf(session->io_cmd);
  strTrimCrlf(session->io_arg);
  //printf("cmd = \"%s\",arg = \"%s\"\n",session->io_cmd,session->io_arg);
}

static void acceptResponse(session_t * const session){
  int accept_count = recv(session->ctrl_fd,session->cmdline,sizeof(session->cmdline),0);
  if(!(accept_count > 0)){
   return;
  }
  char * head = session->cmd;
  char * tail = session->arg;
  strSplit(&head,&tail,session->cmdline," ",1);
  strUpper(session->cmd);
  strTrimCrlf(session->cmd);
  strTrimCrlf(session->arg);
  //printf("\ncmd = \"%s\",arg = \"%s\"\n",session->cmd,session->arg);
}

static void sendCtrlResponse(session_t * const session,const char* cmd,const char* data){
	char send_buff[MAX_SIZE];
    if(data != NULL){
		snprintf(send_buff,sizeof(send_buff),"%s %s\r\n",cmd,data);
    }else{
		snprintf(send_buff,sizeof(send_buff),"%s\r\n",cmd);
	}
	write(session->ctrl_fd,send_buff,strlen(send_buff));
	//printf("send socket %d : %s",session->ctrl_fd,send_buff);
}
volatile int flag = 0;
void* recv_io(void* sess){
	int i;
	const int size = sizeof(ctrl_cmds) / sizeof(ctrl_cmds[0]);
	printf("%d\n",size);
	session_t * session = sess;
	clearIOBuffer(session);
	acceptIOBuffer(session);
	if(strlen(session->io_cmd) == 0){
		return;
	}
	for (i=0; i<size; i++)
	{
		if (strcmp(ctrl_cmds[i].cmd, session->io_cmd) == 0)
		{
			if (ctrl_cmds[i].cmd_handler != NULL)
			{
				ctrl_cmds[i].cmd_handler(session);
			}
			else
			{
				//sendCtrlResponse(session, "ERROR", "Unimplement command.");
			}
			
			break;
		}
	}

	if (i == size)
	{
		printf("%s",session->io_cmd);
		//sendCtrlResponse(session, "ERROR", "Unknown command.");
	}

}
int handles(session_t * const session){
	int i;
	const int size = sizeof(ctrl_cmds) / sizeof(ctrl_cmds[0]);
	printf("%d\n",size);
	memset(session->data_buff,0,sizeof(session->data_buff));
	acceptResponse(session);
	printf("%s\nftp:>",session->arg);
	fflush(stdin);
	fflush(stdout);
	//printf("ftp:>");
	fd_set read_fd,all_fd,exception_fd;
	FD_ZERO( &all_fd);
	FD_ZERO( &exception_fd );
	FD_SET( STDIN, &all_fd);
	FD_SET( session->ctrl_fd, &all_fd);
	pthread_t pid;
	while(1){
		if(session->alive == 0)return 0;
		read_fd = all_fd;
		select( session->ctrl_fd + 1, &read_fd, NULL, &exception_fd, NULL );
		if ( FD_ISSET( STDIN, &read_fd ) ){
			clearIOBuffer(session);
			acceptIOBuffer(session);
			if(strlen(session->io_cmd) == 0){
				//return;
			}
			for (i=0; i<size; i++)
			{
				if (strcmp(ctrl_cmds[i].cmd, session->io_cmd) == 0)
				{
					if (ctrl_cmds[i].cmd_handler != NULL)
					{
						ctrl_cmds[i].cmd_handler(session);
					}
					else
					{
						//sendCtrlResponse(session, "ERROR", "Unimplement command.");
					}
					
					break;
				}
			}

			if (i == size)
			{
				printf("%s",session->io_cmd);
				//sendCtrlResponse(session, "ERROR", "Unknown command.");
			}			
			printf("ftp:>");
			fflush(stdin);
			fflush(stdout);
		}
		if ( FD_ISSET( session->ctrl_fd, &read_fd ) ){
			clearSessionCtrlData(session);
			acceptResponse(session);
			if(strcmp(session->cmd,"666") == 0){
				recv_data_flag = 1;
				printf("\nrecv message: %s\n",session->arg);
				fflush(stdin);
				fflush(stdout);
				printf("ftp:>");
				fflush(stdin);
				fflush(stdout);
				clearIOBuffer(session);
			}
		}
	}
}