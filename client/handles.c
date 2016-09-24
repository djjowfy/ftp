#include "common.h"
#include "handles.h"
#include "file_util.h"

char buff[MAX_SIZE];
struct simple_file recving_file;
struct ipaddr data_ip;
void accept_response(const int sockfd);
int check_response(const char * corret);
void send_cmd(const int sockfd,const char * cmd,char * data);
int pre_ftp(const int sockfd);
void get_ipaddr(void);
int download(const int sockfd,const int data_socket);
int update(const int sockfd,const int data_socket);

int handles(const int sockfd){
    if(pre_ftp(sockfd) != 0){
       return -1;
    }
	while(1){
		printf("do you want download a file ?y or n or q :\n");
		char result[5];
		gets(result);
		 if(strcmp(result,"y") == 0){
			send_cmd(sockfd,"PASV",NULL);
			accept_response(sockfd);
			if(check_response("227")){
		 
			}else{
			   printf("server response PASV error");
			   return -2;
			}
			get_ipaddr();
			const int data_socket = create_client_socket(data_ip.ip,data_ip.port);
			download(sockfd,data_socket);
			close(data_socket); 
		 }else if(strcmp(result,"q") == 0){
			 return 0;
		 }
		 printf("do you want update/store a file ? y or n or q :\n");
		 gets(result);
		 if(strcmp(result,"y") == 0){
			send_cmd(sockfd,"PASV",NULL);
			accept_response(sockfd);
			if(check_response("227")){
			}else{
			 printf("server response PASV error");
			 return -2;
			}
		   get_ipaddr();
		   const int data_socket = create_client_socket(data_ip.ip,data_ip.port);
		   update(sockfd,data_socket);
		   close(data_socket);
		}else if(strcmp(result,"q") == 0){
			 return 0;
		 }
    } 
   return 0;
}

int update(const int sockfd,const int data_socket){
   printf("please input file dir\n");
   gets(recving_file.path);
   send_cmd(sockfd,"CWD",recving_file.path);
   accept_response(sockfd);
   if(check_response("250")){
   }else{
     printf("server response CWD error");
     return -2;
   }
   printf("please input file name\n");
   gets(recving_file.name);
   snprintf(recving_file.abpath,sizeof(recving_file.abpath),"%s%s",recving_file.path,recving_file.name);
   send_cmd(sockfd,"STOR",recving_file.name);
   accept_response(sockfd);
   if(check_response("150")){
   }else{
     printf("server response STOR error");
     return -2;
   }
   send_file(data_socket,recving_file.abpath);
   return 0;
}

int download(const int sockfd,const int data_socket){
   printf("please input file dir\n");
   gets(recving_file.path);
   send_cmd(sockfd,"CWD",recving_file.path);
   accept_response(sockfd);
   if(check_response("250")){
   }else{
     printf("server response CWD error");
     return -2;
   }
   printf("please input file name\n");
   gets(recving_file.name);
      snprintf(recving_file.abpath,sizeof(recving_file.abpath),"%s%s",recving_file.path,recving_file.name);
   send_cmd(sockfd,"SIZE",recving_file.name);
   accept_response(sockfd);
   if(check_response("213")){
   }else{
     printf("server response SIZE error");
     return -2;
   }
   char * data;
   strtok_r(buff," ",&data);
   send_cmd(sockfd,"RETR",recving_file.abpath);
   accept_response(sockfd);
   if(check_response("150")){
   }else{
     printf("server response RETR error");
     return -2;
   }

   recv_file(data_socket,recving_file.abpath);
   return 0;
}

void get_ipaddr(void){
  char *data;
  strtok_r(buff,"(",&data);
  char* ip[4];
  int i;
  for(i = 0;i < 4;i ++){
   ip[i] = strtok_r(data,",",&data);
  }
  char* port_1;
  port_1 = strtok_r(data,",",&data);
  char * port_2_temp;
  port_2_temp = strtok_r(data,",",&data);
  char port_2[4];
  memcpy(port_2,port_2_temp,strlen(port_2_temp) - 1);
  snprintf((data_ip.ip),(sizeof(data_ip.ip)),"%s%s%s%s%s%s%s",ip[0],".",ip[1],".",ip[2],".",ip[3]);
  data_ip.port = atoi(port_1) * 256 + atoi(port_2);
  printf("server data ip is : %s:%d\n",data_ip.ip,data_ip.port);
}
int pre_ftp(const int sockfd){
  accept_response(sockfd);
  if(check_response("220")){
    printf("connect FTP SERVER success!!\n");
  }else{
   printf("connect verify failed!!\n");
   return -1;
  }
  printf("please input username : \n");
  gets(username);
  send_cmd(sockfd,"USER",username);
  accept_response(sockfd);
  if(!check_response("331")){
    printf("user verify failed!!\n");
    return -2;
  }
  printf("please input password : \n");
  gets(password);
  send_cmd(sockfd,"PASS",password);
  accept_response(sockfd);
  if(!check_response("230")){
    return -3;
  }
  return 0;
}

void accept_response(const int sockfd){
  int accept_count = recv(sockfd,buff,MAX_SIZE,0);
  buff[accept_count] = '\0';
  printf("%s\n",buff);
}

void send_cmd(const int sockfd,const char * cmd,char * data){
  char send_buff[MAX_SIZE];
  if(data != NULL){
   snprintf(send_buff,sizeof(send_buff),"%s%s%s%s",cmd," ",data,"\r\n");
  }else{
   snprintf(send_buff,sizeof(send_buff),"%s%s",cmd,"\r\n");
  }
  write(sockfd,send_buff,strlen(send_buff));
  printf("send socket %d : %s",sockfd,send_buff);
}

int check_response(const char* corret){
  if(strlen(buff) < strlen(corret)){
    return 0;
  }
  return ((*buff) == (*corret) && (*(buff + 1)) == (*(corret + 1)) && (*(buff + 2)) == (*(corret + 2)));
}

