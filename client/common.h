#ifndef COMMON_H
#define COMMON_H
#define _GNU_SOURCE
#define _OPEN_SYS_ITOA_EXT
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <netinet/in.h>
#include <time.h>
#include <dirent.h>

#define FILE_NAME_MAX_SIZE 512
#define MAX_COMMAND_LINE 1024
#define MAX_COMMAND 32
#define MAX_ARG 1024
#define MAX_SIZE 1024
#define MAX_FILE_COUNT 512 //最多文件文件夹数目
#define USERNAME "djjowfy"
#define PASSWORD "djjowfy"
#define MAX_CLIENT 16

char username[100];
char password[100];
struct ipaddr{
  char ip[100];
  int port;
};

struct simple_file{
  char path[FILE_NAME_MAX_SIZE];
  char name[FILE_NAME_MAX_SIZE];
  char abpath[FILE_NAME_MAX_SIZE*2];
  char size[100];
};

typedef struct ftp_session
{
	// 控制连接
	int ctrl_fd;//命令socket
	char cmdline[MAX_COMMAND_LINE];//接收到的命令行（命令 + 参数）
	char cmd[MAX_COMMAND];//接受到的cmd
	char arg[MAX_ARG];//接收到的cmd所带的参数

	// 数据连接
	int pasv_listen_fd;
	int data_fd;
    char data_buff[MAX_ARG];
	char data_ip[32];
	int data_port;
	
	//状态
	int is_login;//登陆为1 
    int type;//0 ascii 1 EBCDIC 2 EBCDIC 3 local format
	char work_path[MAX_ARG];//当前路径
	void * self;//指向自身的指针
	void * pre;//指向前面的指针
	void * next;//指向后面的指针
	int alive;//是否存活
	volatile int modifier;//是否有新信息
	//身份
	char name[100];
	
	//IO缓存
	char ioline[MAX_COMMAND_LINE];//接收到的命令行（命令 + 参数）
	char io_cmd[MAX_COMMAND];//接受到的cmd
	char io_arg[MAX_ARG];//接收到的cmd所带的参数
}session_t;


typedef struct user{
  char name[100];
  char password[100];
}user_t;

user_t users[100];
#endif
