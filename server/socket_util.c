#include "common.h"
#include "socket_util.h"
int create_client_socket(const char* ip_address,int port){
  /* create a socket */  
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);  
    
  struct sockaddr_in address;  
  address.sin_family = AF_INET;  
  address.sin_addr.s_addr = inet_addr(ip_address);  
  address.sin_port = htons(port);  
    
  /* connect to the server */  
  int result = connect(sockfd, (struct sockaddr *)&address, sizeof(address));  
  if(result == -1)  
  {  
    perror("connect failed: ");  
    exit(1);  
  } 
  return sockfd;

}
int create_server_socket(int port)
{
    int sock;
    int reuse = 1;
    struct sockaddr_in server_address = (struct sockaddr_in){
        AF_INET,//ipv4
        htons(port),
        (struct in_addr){INADDR_ANY}//0.0.0.0
    };


    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){//socket(int domain, int type, int protocol)
        fprintf(stderr, "Cannot open socket");
        exit(EXIT_FAILURE);
    }

    /* Address can be reused instantly after program exits */
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse);

    /* Bind socket to server address */
    if(bind(sock,(struct sockaddr*) &server_address, sizeof(server_address)) < 0){
        fprintf(stderr,"Cannot bind socket to address\n");
        exit(EXIT_FAILURE);
    }

    listen(sock,5);
    return sock;
}
