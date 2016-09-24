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
#define MAX_SIZE 1024
#define USERNAME "djjowfy"
#define PASSWORD "djjowfy"
struct user{
  char * name;
  char * password;
};
struct simple_file{
  char path[FILE_NAME_MAX_SIZE];
  char name[FILE_NAME_MAX_SIZE];
  char abpath[FILE_NAME_MAX_SIZE*2];
  char size[100];
};
#define REQIURE_PASS "331 User name okay,need password"
#define TELL_LOGIN "230 User logged in proceed"
#define PASV_RESPONSE "227 Entering passive mode(104,224,166,224,"
#define CWD_RESPONSE "250 Command okay."
#define SIZE_RESPONSE "213"
#define RETR_RESPONSE "150 Opening data connection."
#endif
