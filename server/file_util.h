#ifndef FILE_UTIL_H
#define FILE_UTIL_H
extern unsigned long get_file_size(const char *path);
extern int send_file(const int sockfd,const char *path);
extern int recv_file(const int sockfd,const char *path);
extern int create_dir(const char *sPathName);
#endif
