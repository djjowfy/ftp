#include <sys/stat.h>  
#include "file_util.h"
#include "common.h"
  
unsigned long get_file_size(const char *path)  
{  
    unsigned long filesize = -1;      
    struct stat statbuff;  
    if(stat(path, &statbuff) < 0){  
        return filesize;  
    }else{  
        filesize = statbuff.st_size;  
    }  
    return filesize;  
}

void recv_file(const int socketfd,const char *path){
    char buffer[MAX_SIZE];
    char file_name[FILE_NAME_MAX_SIZE+1];
    bzero(file_name, FILE_NAME_MAX_SIZE+1);
    strncpy(file_name, path, strlen(path)>FILE_NAME_MAX_SIZE?FILE_NAME_MAX_SIZE:strlen(path));
    printf("%s\n", file_name);
    FILE *fp = fopen(file_name ,"w");  
    if(NULL == fp)  
    {  
        printf("File:\t%s Can Not Open To Write\n",file_name);  
        exit(1);  
    }  
  
    bzero(buffer, MAX_SIZE);  
    int recv_count;  
    while(recv_count = recv(socketfd, buffer, MAX_SIZE, 0) > 0)  
    {  
        printf("recv count:%d",recv_count);
        //recv_count = recv(socketfd, buffer, MAX_SIZE, 0);
        if(fwrite(buffer, sizeof(char), recv_count, fp) < recv_count)  
        {  
            printf("File:\t%s Write Failed\n", file_name);  
            break;  
        }  
        bzero(buffer, MAX_SIZE);  
    }
    printf("Receive File:\t%s From Server IP Successful!\n", file_name);  
    fclose(fp);  
}

int send_file(const int sockfd,const char *path){
        char buffer[MAX_SIZE];
        char file_name[FILE_NAME_MAX_SIZE+1];  
        bzero(file_name, FILE_NAME_MAX_SIZE+1);  
        strncpy(file_name, path, strlen(path)>FILE_NAME_MAX_SIZE?FILE_NAME_MAX_SIZE:strlen(path));  
        printf("%s\n", file_name); 
FILE *fp = fopen(file_name, "r");  
        if(NULL == fp)  
        {  
            printf("File:%s Not Found\n", file_name);  
        }  
        else  
        {  
            bzero(buffer, MAX_SIZE);  
            int length = 0;  
            while((length = fread(buffer, sizeof(char), MAX_SIZE, fp)) > 0)  
            {  
                if(send(sockfd, buffer, length, 0) < 0)  
                {  
                    printf("Send File:%s Failed./n", file_name);  
                    break;  
                }  
                bzero(buffer, MAX_SIZE);  
            }  
  
            fclose(fp);  
            printf("File:%s Transfer Successful!\n", file_name);  
        }  
}  
