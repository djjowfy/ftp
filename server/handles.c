#include "common.h"
#include "handles.h"
#include "socket_util.h"
#include "file_util.h"

char buff[MAX_SIZE];
char* cmd;
char* data;
struct user client_user;
struct simple_file sending_file;
void accept_response(const int sockfd);
int check_response(const char * corret);
void send_cmd(const int sockfd,const char * cmd,char * data);
void send_response(const int sockfd,const char * response,const char * data);
int verify_login(const int sockfd);

int handles(const int sockfd){
  if(verify_login(sockfd) != 0){
     return -1;
   }
   while(1){
   accept_response(sockfd);
   if(check_response("PASV")){

   }else{
    printf("PASV error");
     return -1;
   }
   send_response(sockfd,PASV_RESPONSE,NULL);
   struct sockaddr_in client_address;
   int len = sizeof(client_address);
   const int socket = create_server_socket(1042);
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
     snprintf(sending_file.abpath,sizeof(sending_file.abpath),"%s%s",sending_file.path,sending_file.name);
	 update(sockfd,data_socket);
   }else{
	   return -1;
   }
   }
   close(socket);
//   close(data_socket);
   return 0;
}

int update(const int sockfd,const int data_socket){
   send_response(sockfd,RETR_RESPONSE,NULL);
   recv_file(data_socket,sending_file.abpath);
   return 0;
}

int download(const int sockfd,const int data_socket){
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
  }else{
    free(client_user.password);
    client_user.password = NULL;
    free(client_user.name);
    client_user.name = NULL;
    return -2;
  }
  send_response(sockfd,TELL_LOGIN,NULL);
  return 0;

}

void send_response(const int sockfd,const char* response,const char* data){
   char sendbuff[1024];
   if(data == NULL){
      printf("%s\n",response);
      write(sockfd,response,strlen(response));
   }else{
     snprintf(sendbuff,sizeof(sendbuff),"%s%s%s",response," ",data);
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
}

