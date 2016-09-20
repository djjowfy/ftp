#ifndef FILE_UTIL_H
#define FILE_UTIL_H
extern unsigned long get_file_size(const char *path);
extern int send_file(const int sockfd,const char *path);
extern void recv_file(const int socketfd,const char *path);
#endif
