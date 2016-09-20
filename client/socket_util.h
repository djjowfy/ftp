#ifndef SOCKET_UTIL_H
#define SOCKET_UTIL_H
extern int create_server_socket(int port);
extern int create_client_socket(const char* ip_address,int port);
#endif
