#include "common.h"
#include "handles.h"
#include "socket_util.h"
#include "file_util.h"
#include "str_utils.h"
#include "ftp_codes.h"
#include "list_dir.h"

/* char buff[MAX_SIZE];
char* cmd;
char* data;
struct user client_user;
struct simple_file sending_file;
void accept_response(const int sockfd);
int check_response(const char * corret);
void send_cmd(const int sockfd,const char * cmd,char * data);
void send_response(const int sockfd,const char * response,const char * data);
int verify_login(const int sockfd); */
static int data_port = 1043;



typedef struct ftpcmd
{
	const char *cmd;
	void (*cmd_handler)(session_t * const session);
} ftpcmd_t;

void* waitDataConnection(void* session);
void* sendFileDir(void* session);
void* sendFile(void* session);
static void clearSessionCtrlData(session_t * const session);
static void acceptResponse(session_t * const session);
static void sendCtrlResponse(session_t * const session,const int response_code,const char* data);
static void userCmdHandler(session_t * const session);
static void passCmdHandler(session_t * const session);
static void pasvCmdHandler(session_t * const session);
static void pwdCmdHandler(session_t * const session);
static void systCmdHandler(session_t * const session);
static void typeCmdHandler(session_t * const session);
static void mkdCmdHandler(session_t * const session);
static void listCmdHandler(session_t * const session);
static void sizeCmdHandler(session_t * const session);
static void cwdCmdHandler(session_t * const session);
static void quitCmdHandler(session_t * const session);
static void retrCmdHandler(session_t * const session);
void sendData(session_t * const session);

//from https://zh.wikipedia.org/wiki/FTP%E5%91%BD%E4%BB%A4%E5%88%97%E8%A1%A8
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
	{"CWD",cwdCmdHandler},//改变工作目录
	{"DELE",NULL},//删除文件
	{"ENC",NULL},//隐私保护通道
	{"EPRT",NULL},//为服务器指定要连接的扩展地址和端口
	{"EPSV",NULL},//进入扩展被动模式
	{"FEAT",NULL},//获得服务器支持的特性列表
	{"HELP",NULL},//如果指定了命令，返回命令使用文档；否则返回一个通用帮助文档
	{"LANG",NULL},//语言协商
	{"LIST",listCmdHandler},//如果指定了文件或目录，返回其信息；否则返回当前工作目录的信息
	{"LPRT",NULL},//为服务器指定要连接的长地址和端口
	{"LPSV",NULL},//进入长被动模式
	{"MDTM",NULL},//返回指定文件的最后修改时间
	{"MIC",NULL},//完整性保护命令
	{"MKD",mkdCmdHandler},//创建目录
	{"MLSD",NULL},//如果目录被命名，列出目录的内容
	{"MLST",NULL},//提供命令行指定的对象的数据
	{"MODE",NULL},//设定传输模式（流、块或压缩）
	{"NLST",NULL},//返回指定目录的文件名列表
	{"NOOP",NULL},//无操作（哑包；通常用来保活）
	{"OPTS",NULL},//为特性选择选项
	{"PASS",passCmdHandler},//认证密码
	{"PASV",pasvCmdHandler},//进入被动模式
	{"PBSZ",NULL},//保护缓冲大小
	{"PORT",NULL},//指定服务器要连接的地址和端口
	{"PROT",NULL},//数据通道保护级别
	{"PWD",pwdCmdHandler},//打印工作目录，返回主机的当前目录
	{"QUIT",quitCmdHandler},//断开连接
	{"REIN",NULL},//重新初始化连接
	{"REST",NULL},//从指定点重新开始传输
	{"RETR",retrCmdHandler},//传输文件副本
	{"RMD",NULL},//删除目录
	{"RNFR",NULL},//从...重命名
	{"RNTO",NULL},//重命名到...
	{"SITE",NULL},//发送站点特殊命令到远端服务器
	{"SIZE",sizeCmdHandler},//返回文件大小
	{"SMNT",NULL},//挂载文件结构
	{"STAT",NULL},//返回当前状态
	{"STOR",NULL},//接收数据并且在服务器站点保存为文件
	{"STOU",NULL},//唯一地保存文件
	{"STRU",NULL},//设定文件传输结构
	{"SYST",systCmdHandler},//返回系统类型
	{"TYPE",typeCmdHandler},//设定传输模式（ASCII/二进制).
	{"USER",userCmdHandler},//认证用户名
	{"XCUP",NULL},//改变之当前工作目录的父目录
	{"XMKD",NULL},//创建目录
	{"XPWD",NULL},//打印当前工作目录
	{"XRCP",NULL},//
	{"XRMD",NULL},//删除目录
	{"XRSQ",NULL},//
	{"XSEM",NULL},//发送，否则邮件
	{"XSEN",NULL}//发送到终端
};



static void clearSessionCtrlData(session_t * const session){
	memset(session->cmdline, 0, sizeof(session->cmdline));
	memset(session->cmd, 0, sizeof(session->cmd));
	memset(session->arg, 0, sizeof(session->arg));
}
static void acceptResponse(session_t * const session){
  int accept_count = recv(session->ctrl_fd,session->cmdline,sizeof(session->cmdline),0);
  //printf("rcv count %d\n",accept_count);
  if(!(accept_count > 0)){
   return;
  }
  char * head = session->cmd;
  char * tail = session->arg;
  strSplit(&head,&tail,session->cmdline," ",1);
  strUpper(session->cmd);
  strTrimCrlf(session->cmd);
  strTrimCrlf(session->arg);
  printf("cmd = \"%s\",arg = \"%s\"\n",session->cmd,session->arg);
}


static void sendCtrlResponse(session_t * const session,const int response_code,const char* data){
   char sendbuff[1024];
   if(data == NULL){
      printf("%d\n",response_code);
	  snprintf(sendbuff,sizeof(sendbuff),"%d\r\n",response_code);
      write(session->ctrl_fd,sendbuff,strlen(sendbuff));
   }else{
     snprintf(sendbuff,sizeof(sendbuff),"%d%s%s\r\n",response_code," ",data);
     printf("%s\n",sendbuff);
     write(session->ctrl_fd,sendbuff,strlen(sendbuff));
   }
}

static void userCmdHandler(session_t * const session){
	session->is_login = 0;
	if(strcmp(session->arg,USERNAME) != 0){
		sendCtrlResponse(session,FTP_LOGINERR,"User is not exit");
	}else{
		sendCtrlResponse(session,FTP_NEEDPWD,"User name okay,need password");
	}
}

static void passCmdHandler(session_t * const session){
	session->is_login = 0;
	if(strcmp(session->arg,PASSWORD) != 0){
		sendCtrlResponse(session,FTP_LOGINERR,"password is not correct");
	}else{
		sendCtrlResponse(session,FTP_LOGINOK,"User logged in, proceed");
		(session->is_login) = 1;
	}
}

static void quitCmdHandler(session_t * const session){
	if(session->is_login == 0){
		sendCtrlResponse(session,FTP_LOGINERR,"please login first");
		return;
	}
	sendCtrlResponse(session,FTP_GREET,"goodbye");
	close(session->ctrl_fd);
}

static void pasvCmdHandler(session_t * const session){
	if(session->is_login == 0){
		sendCtrlResponse(session,FTP_LOGINERR,"please login first");
		return;
	}

    data_port ++;
    session->pasv_listen_fd = create_server_socket(data_port);
	char buff[255];
	memset(buff ,0, sizeof(buff));
    snprintf(buff,sizeof(buff),"%s%d%s%d%s",PASV_RESPONSE,data_port/256,",",data_port%256,")");
	sendCtrlResponse(session,FTP_PASVOK,buff);
	pthread_t pid;
    pthread_create(&pid,NULL,waitDataConnection,(void *)session);
}

void* waitDataConnection(void* session){
	
	struct sockaddr_in client_address;
    int len = sizeof(client_address);
	session_t *sess = session;
	printf("pasv_listen_fd : %d\n",sess->pasv_listen_fd );
	sess->data_fd = accept(sess->pasv_listen_fd,(struct sockaddr*) &client_address,&len);
    printf("data_fd : %d \n",sess->data_fd );
	return NULL;
}

static void pwdCmdHandler(session_t * const session){
	if(session->is_login == 0){
		sendCtrlResponse(session,FTP_LOGINERR,"please login first");
		return;
	}
	char text[MAX_ARG + 2] = {0};
    memset(session->work_path,0,sizeof(session->work_path));
   // strcpy(session->work_path,"/etc/share/");
	getcwd(session->work_path, MAX_ARG - 1);//得到现在的工作路径
	snprintf(text,sizeof(text),"\"%s\"",session->work_path);
	sendCtrlResponse(session,FTP_PWDOK,text);
}

static void systCmdHandler(session_t * const session){
	if(session->is_login == 0){
		sendCtrlResponse(session,FTP_LOGINERR,"please login first");
		return;
	}
	sendCtrlResponse(session,FTP_SYSTOK,SYST_RESPONSE);
}

static void cwdCmdHandler(session_t * const session){
	if(session->is_login == 0){
		sendCtrlResponse(session,FTP_LOGINERR,"please login first");
		return;
	}
    if(chdir(session->arg) < 0){
        sendCtrlResponse(session,FTP_FILEFAIL,"can not change the directory");
        return;
    }
	sendCtrlResponse(session,FTP_CWDOK,"Command okay");
}
static void sizeCmdHandler(session_t * const session){
	if(session->is_login == 0){
		sendCtrlResponse(session,FTP_LOGINERR,"please login first");
		return;
	}
	const unsigned long ulong_value = get_file_size(session->arg);
	if(ulong_value == -1){
		sendCtrlResponse(session,FTP_FILEFAIL,"no exit");
		return;
	}
	const int n = snprintf(NULL, 0, "%lu", ulong_value);
	char buf[n+1];
	snprintf(buf, n+1, "%lu", ulong_value);
	sendCtrlResponse(session,FTP_SIZEOK,buf);
}

static void typeCmdHandler(session_t * const session){
	if(session->is_login == 0){
		sendCtrlResponse(session,FTP_LOGINERR,"please login first");
		return;
	}
	if (strcmp(session->arg, "A") == 0)
	{
		session->type = 0;
		sendCtrlResponse(session, FTP_TYPEOK, "Switching to ASCII mode.");
	}else if (strcmp(session->arg, "E") == 0)
	{
		session->type = 1;
		sendCtrlResponse(session, FTP_TYPEOK, "Switching to EBCDIC mode.");
	}else if (strcmp(session->arg, "I") == 0)
	{
		session->type = 2;
		sendCtrlResponse(session, FTP_TYPEOK, "Switching to Binary mode.");
	}
	else if (strcmp(session->arg, "L") == 0)
	{
		session->type = 3;
		sendCtrlResponse(session, FTP_TYPEOK, "Switching to local format mode.");
	}
	else
	{
		sendCtrlResponse(session, FTP_BADMODE, "Unrecognised TYPE command.");
	}
	
}

static void mkdCmdHandler(session_t * const session){
	if(session->is_login == 0){
		sendCtrlResponse(session,FTP_LOGINERR,"please login first");
		return;
	}
	if(mkdir(session->arg,0755) == -1){
		sendCtrlResponse(session,FTP_FILEFAIL,"Create direction operator failed");
	}else{
		char buff[1024] = {0};
	    snprintf(buff,sizeof(buff),"\"%s\"",session->arg);
		sendCtrlResponse(session,FTP_MKDIROK,buff);
	}
	
}

static void listCmdHandler(session_t * const session){
	if(session->is_login == 0){
		sendCtrlResponse(session,FTP_LOGINERR,"please login first");
		return;
	}
	struct timeval tv;   
    gettimeofday(&tv, NULL);    
    long now_time = tv.tv_sec;
	while(session->data_fd == -1){
		gettimeofday(&tv, NULL);
		if(((tv.tv_sec) - now_time) > 30){//等待30s
			sendCtrlResponse(session,FTP_DATATLSBAD," data sockect is not working well");
			return;
		}
	}
	sendCtrlResponse(session,FTP_DATACONN," Here comes the directory listing.");
	
	memset(session->data_buffer,0,sizeof(session->data_buffer));
	if(listDir(session->arg,session->data_buffer,session->work_path) == -1){
		sendCtrlResponse(session,FTP_FILEFAIL,"cannot list the directory");
	}else{
		pthread_t pid;
		pthread_create(&pid,NULL,sendFileDir,(void *)session);
	}
	
}

static void retrCmdHandler(session_t * const session){
	if(session->is_login == 0){
		sendCtrlResponse(session,FTP_LOGINERR,"please login first");
		return;
	}
	struct timeval tv;   
    gettimeofday(&tv, NULL);    
    long now_time = tv.tv_sec;
	while(session->data_fd == -1){
		gettimeofday(&tv, NULL);
		if(((tv.tv_sec) - now_time) > 30){//等待30s
			sendCtrlResponse(session,FTP_DATATLSBAD," data sockect is not working well");
			return;
		}
	}

	sendCtrlResponse(session,FTP_DATACONN," Here comes the file data.");
	pthread_t pid;
	pthread_create(&pid,NULL,sendFile,(void *)session);
}

void* sendFile(void* session){
	session_t *sess = session;
	if(send_file(sess->data_fd,sess->arg) == -1 ){
		sendCtrlResponse(sess,FTP_FILEFAIL,"cannot return the file");
	}else{
		sendCtrlResponse(sess,FTP_TRANSFEROK,"trans ok");
	}	
	close(sess->data_fd);
	close(sess->pasv_listen_fd);
	sess->data_fd = -1;
	sess->pasv_listen_fd = -1;
    return NULL;
}

void sendData(session_t * const session){
	write(session->data_fd,session->data_buffer,strlen(session->data_buffer));
}

void* sendFileDir(void* session){
	session_t *sess = session;
	sendData(sess);
	printf("data_socket %d\n %s",sess->data_fd,sess->data_buffer);
	close(sess->data_fd);
	close(sess->pasv_listen_fd);
	sess->data_fd = -1;
	sess->pasv_listen_fd = -1;
	sendCtrlResponse(session,FTP_TRANSFEROK," list the directory");
    return NULL;
}

int handles(session_t * const session){
	int i;
	const int size = sizeof(ctrl_cmds) / sizeof(ctrl_cmds[0]);
	printf("%d",size);
	while(1){
		clearSessionCtrlData(session);
		acceptResponse(session);
		if(strlen(session->cmd) == 0){
			continue;
		}
		for (i=0; i<size; i++)
		{
			if (strcmp(ctrl_cmds[i].cmd, session->cmd) == 0)
			{
				if (ctrl_cmds[i].cmd_handler != NULL)
				{
					ctrl_cmds[i].cmd_handler(session);
				}
				else
				{
					sendCtrlResponse(session, FTP_COMMANDNOTIMPL, "Unimplement command.");
				}
				
				break;
			}
		}

		if (i == size)
		{
			printf("%s",session->cmd);
			sendCtrlResponse(session, FTP_BADCMD, "Unknown command.");
		}
	}
}

/*   if(verify_login(sockfd) != 0){
    close(sockfd);
     return -1;
   }
   while(1){
	   accept_response(sockfd);
	   if(check_response("PASV")){

	   }else{
			printf("PASV error");
			return -1;
	   }
	   struct sockaddr_in client_address;
	   int len = sizeof(client_address);
	   data_port ++;
	   const int socket = create_server_socket(data_port);
	   snprintf(buff,sizeof(buff),"%s%d%s%d%s",PASV_RESPONSE,data_port/256,",",data_port%256,")");
	   send_response(sockfd,buff,NULL);
	   const int data_socket = accept(socket,(struct sockaddr*) &client_address,&len);
	   accept_response(sockfd);
	   if(!check_response("CWD")){
			printf("CWD error");
			return -1;
	   }
	   memcpy(sending_file.path,data,strlen(data)-2);
	   send_response(sockfd,CWD_RESPONSE,NULL);
	   accept_response(sockfd);
	   if(check_response("SIZE")){
		    memcpy(sending_file.name,data,strlen(data)-2);
		    snprintf(sending_file.abpath,sizeof(sending_file.abpath),"%s%s",sending_file.path,sending_file.name);
		    download(sockfd,data_socket);
	   }else if(check_response("STOR")){
			memcpy(sending_file.name,data,strlen(data)-2);
			printf("%s\n",sending_file.name);
			snprintf(sending_file.abpath,sizeof(sending_file.abpath),"%s%s",sending_file.path,sending_file.name);
			update(sockfd,data_socket);
	   }else{
		   	close(socket);
		    close(data_socket);
		    return -1;
	   }
	    close(socket);
		close(data_socket);
   }
   close(sockfd);
   return 0;
} */

/* int update(const int sockfd,const int data_socket){
   if(opendir(sending_file.path) == NULL){
	   create_dir(sending_file.path);
   }
   send_response(sockfd,RETR_RESPONSE,NULL);
   recv_file(data_socket,sending_file.abpath);
   return 0;
}

int download(const int sockfd,const int data_socket){
	if(access(sending_file.abpath,F_OK) != 0){
		printf("flie %s not find",sending_file.abpath);
		send_response(sockfd,ERR_RESPONSE,"not find file");
        return -1;
	}
   snprintf(sending_file.size,sizeof(sending_file.size),"%lu",get_file_size(sending_file.abpath));
   send_response(sockfd,SIZE_RESPONSE,sending_file.size);
   accept_response(sockfd);
   if(!check_response("RETR")){
     printf("SIZE error");
     return -1;
   }
   send_response(sockfd,RETR_RESPONSE,NULL);
   send_file(data_socket,sending_file.abpath);
}

int verify_login(const int sockfd){
  accept_response(sockfd);
  if(check_response("USER")){
   printf("username %s",data);
   client_user.name = (char *)malloc(sizeof(char)*(strlen(data) - 2));
   memcpy(client_user.name,data,strlen(data) - 2);
   if(strcmp(client_user.name,USERNAME) != 0)return -1;
  }else{
   printf("username error");
   free(client_user.name);
   client_user.name = NULL;
   return -1;
  }
  send_response(sockfd,REQIURE_PASS,NULL);
  accept_response(sockfd);
  if(check_response("PASS")){
    printf("password %s",data);
    client_user.password = (char *)malloc(sizeof(char) * (strlen(data) - 2));
    memcpy(client_user.password,data,strlen(data) -  2);
    if(strcmp(client_user.password,PASSWORD)!=0)return -1;
  }else{
    free(client_user.password);
    client_user.password = NULL;
    free(client_user.name);
    client_user.name = NULL;
    return -2;
  }
  send_response(sockfd,TELL_LOGIN,NULL);
  accept_response(sockfd);
  if(check_response("SYST")){
    
  }else{
    return -2;
  }
 send_response(sockfd,SYST_RESPONSE,NULL);
  return 0;

}

void send_response(const int sockfd,const char* response,const char* data){
   char sendbuff[1024];
   if(data == NULL){
      printf("%s\n",response);
      snprintf(sendbuff,sizeof(sendbuff),"%s\r\n",response);
      write(sockfd,sendbuff,strlen(sendbuff));
   }else{
     snprintf(sendbuff,sizeof(sendbuff),"%s%s%s\r\n",response," ",data);
     printf("%s\n",sendbuff);
     write(sockfd,sendbuff,strlen(sendbuff));
   }
}

void accept_response(const int sockfd){
  int accept_count = recv(sockfd,buff,MAX_SIZE,0);
  printf("rcv count %d\n",accept_count);
  if(accept_count == 0){
   return;
  }
  buff[accept_count] = '\0';
  printf("%s",buff);
  if(strstr(buff," ") == NULL){
    cmd = strtok_r(buff,"\r",&data); 
  }
  cmd = strtok_r(buff," ",&data);
}

void send_cmd(const int sockfd,const char * cmd,char * data){
  char send_buff[MAX_SIZE];
  strcpy(send_buff,cmd);
  strcpy(send_buff,data);
  strcpy(send_buff,"\r\n");
  strcpy(send_buff,"\0");
  write(sockfd,send_buff,strlen(send_buff));
}

int check_response(const char* corret){
  if(strlen(buff) < strlen(corret)){
     return 0;
  }
  int i = 0;
  for(i = 0;i < strlen(cmd);i ++){
    if((*(buff + i)) != (*(corret + i))){
       return 0;
     }
  }
  return 1;
} */

