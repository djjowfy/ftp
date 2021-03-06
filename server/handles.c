#include "common.h"
#include "handles.h"
#include "socket_util.h"
#include "file_util.h"
#include "str_utils.h"
#include "ftp_codes.h"
#include "list_dir.h"

static int data_port = 1043;



typedef struct ftpcmd
{
	const char *cmd;
	void (*cmd_handler)(session_t * const session);
} ftpcmd_t;

void* waitDataConnection(void* session);
void* sendFileDir(void* session);
void* sendFile(void* session);
void* recvFile(void* session);

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
static void storCmdHandler(session_t * const session);
static void siteCmdHandler(session_t * const session);
static void cdupCmdHandler(session_t * const session);

//from https://zh.wikipedia.org/wiki/FTP%E5%91%BD%E4%BB%A4%E5%88%97%E8%A1%A8
static ftpcmd_t ctrl_cmds[] = {
	{"ABOR",NULL},//(ABORT)此命令使服务器终止前一个FTP服务命令以及任何相关数据传输。
	{"ACCT",NULL},//(ACCOUNT)此命令的参数部分使用一个Telnet字符串来指明用户的账户。
	{"ADAT",NULL},//(AUTHENTICATION/SECURITY DATA)认证/安全数据
	{"ALLO",NULL},//为接收一个文件分配足够的磁盘空间
	{"APPE",NULL},//增加
	{"AUTH",NULL},//认证/安全机制
	{"CCC",NULL},//清除命令通道
	{"CDUP",cdupCmdHandler},//改变到父目录
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
	{"SITE",siteCmdHandler},//发送站点特殊命令到远端服务器
	{"SIZE",sizeCmdHandler},//返回文件大小
	{"SMNT",NULL},//挂载文件结构
	{"STAT",NULL},//返回当前状态
	{"STOR",storCmdHandler},//接收数据并且在服务器站点保存为文件
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

static void cdupCmdHandler(session_t * const session){
	if(session->is_login == 0){
		sendCtrlResponse(session,FTP_LOGINERR,"please login first");
		return;
	}
    if(chdir("..") < 0){
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
	    snprintf(buff,sizeof(buff),"\"%s/%s\" created",session->work_path,session->arg);
		sendCtrlResponse(session,FTP_MKDIROK,buff);
	}
	
}

static void siteCmdHandler(session_t * const session){
	if(session->is_login == 0){
		sendCtrlResponse(session,FTP_LOGINERR,"please login first");
		return;
	}
	char *left;
	left = session->arg;
	while((*left) != ' '){
		*left = tolower(*left);
		left ++;
	}
	int ret = system(session->arg);
	if(ret == -1){
		sendCtrlResponse(session, FTP_BADSTRU,"fork error");
	}else if(ret == 127){
		sendCtrlResponse(session,FTP_BADCMD,"command error");
	}else{
		sendCtrlResponse(session,FTP_SITECMDOK,"command OK");
	}
	
}

static void listCmdHandler(session_t * const session){
	if(session->is_login == 0){
		sendCtrlResponse(session,FTP_LOGINERR,"please login first");
		return;
	}
	strcpy(session->data_buff,session->arg);
	struct timeval tv;   
    gettimeofday(&tv, NULL);    
    long now_time = tv.tv_sec;
	while(session->data_fd == -1){
		gettimeofday(&tv, NULL);
		if(((tv.tv_sec) - now_time) > 30){//等待30s
			sendCtrlResponse(session,FTP_DATATLSBAD," data sockect is not working well");
			memset(session->data_buff,0,sizeof(session->data_buff));
			if((session->pasv_listen_fd) != -1){
				close(session->pasv_listen_fd);
			}
			return;
		}
	}	

	pthread_t pid;
	pthread_create(&pid,NULL,sendFileDir,(void *)session);
	sendCtrlResponse(session,FTP_DATACONN," Here comes the directory listing.");
	
}

void* sendFileDir(void* session){
	session_t *sess = session;
	if(listDir(sess) == -1 ){
		sendCtrlResponse(sess,FTP_FILEFAIL,"cannot list the directory");
	}else{
		sendCtrlResponse(sess,FTP_TRANSFEROK,"trans ok");
	}	
	close(sess->data_fd);
	close(sess->pasv_listen_fd);
	sess->data_fd = -1;
	sess->pasv_listen_fd = -1;
	memset(sess->data_buff,0,sizeof(sess->data_buff));
    return NULL;
}

static void retrCmdHandler(session_t * const session){
	if(session->is_login == 0){
		sendCtrlResponse(session,FTP_LOGINERR,"please login first");
		return;
	}
	strcpy(session->data_buff,session->arg);
	struct timeval tv;   
    gettimeofday(&tv, NULL);    
    long now_time = tv.tv_sec;
	while(session->data_fd == -1){
		gettimeofday(&tv, NULL);
		if(((tv.tv_sec) - now_time) > 30){//等待30s
			sendCtrlResponse(session,FTP_DATATLSBAD," data sockect is not working well");
			memset(session->data_buff,0,sizeof(session->data_buff));
			if(session->pasv_listen_fd != -1){
				close(session->pasv_listen_fd);
			}
			return;
		}
	}
	pthread_t pid;
	pthread_create(&pid,NULL,sendFile,(void *)session);
	sendCtrlResponse(session,FTP_DATACONN," Here comes the file data.");
}

void* sendFile(void* session){
	session_t *sess = session;
	if(send_file(sess->data_fd,sess->data_buff) == -1 ){
		sendCtrlResponse(sess,FTP_FILEFAIL,"cannot return the file");
	}else{
		sendCtrlResponse(sess,FTP_TRANSFEROK,"trans ok");
	}	
	close(sess->data_fd);
	close(sess->pasv_listen_fd);
	sess->data_fd = -1;
	sess->pasv_listen_fd = -1;
    memset(sess->data_buff,0,sizeof(sess->data_buff));
    return NULL;
}

static void storCmdHandler(session_t * const session){
	if(session->is_login == 0){
		sendCtrlResponse(session,FTP_LOGINERR,"please login first");
		return;
	}
	strcpy(session->data_buff,session->arg);
	struct timeval tv;   
    gettimeofday(&tv, NULL);    
    long now_time = tv.tv_sec;
	while(session->data_fd == -1){
		gettimeofday(&tv, NULL);
		if(((tv.tv_sec) - now_time) > 30){//等待30s
			sendCtrlResponse(session,FTP_DATATLSBAD," data sockect is not working well");
			memset(session->data_buff,0,sizeof(session->data_buff));
			if((session->pasv_listen_fd) != -1){
				close(session->pasv_listen_fd);
			}
			return;
		}
	}
	pthread_t pid;
	pthread_create(&pid,NULL,recvFile,(void *)session);
	sendCtrlResponse(session,FTP_DATACONN," Here is ready");
}

void* recvFile(void* session){
	session_t *sess = session;
	if(recv_file(sess->data_fd,sess->data_buff) == -1 ){
		sendCtrlResponse(sess,FTP_FILEFAIL,"cannot return the file");
	}else{
		sendCtrlResponse(sess,FTP_TRANSFEROK,"trans ok");
	}	
	close(sess->data_fd);
	close(sess->pasv_listen_fd);
	sess->data_fd = -1;
	sess->pasv_listen_fd = -1;
    memset(sess->data_buff,0,sizeof(sess->data_buff));
    return NULL;
}


int handles(session_t * const session){
	int i;
	const int size = sizeof(ctrl_cmds) / sizeof(ctrl_cmds[0]);
	printf("%d",size);
	memset(session->data_buff,0,sizeof(session->data_buff));
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



