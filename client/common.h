#ifndef COMMON_H
#define COMMON_H
#define _GNU_SOURCE
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

#define USERNAME "djjowfy"
#define PASSWORD "djjowfy"
#define REV_TIME_OUT_S 3
#define REV_TIME_OUT_MS 0
#define MAX_SIZE 1024
#define FILE_NAME_MAX_SIZE 512

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
#endif
