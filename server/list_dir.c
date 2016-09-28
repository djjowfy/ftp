#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <grp.h>
#include <pwd.h>
#include "list_dir.h"
#include "str_utils.h"

#define DETAIL (1<<0)  /*带l参数标志*/  
#define INODE (1<<1)  /*带i参数标志*/   
#define ALL (1<<16) /*带a参数标志*/  
#define NOALL (1<<17) /*带A参数标志*/ 

#define LENGTH 1024 /*字符串最大长度*/  
#define FILECOUNT 100//文件和文件夹最大数目
const char *statbuf_get_perms(struct stat *sbuf);
const char *statbuf_get_date(struct stat *sbuf);
const char *statbuf_get_filename(struct stat *sbuf, const char *name);
const char *statbuf_get_user_info(struct stat *sbuf);
const char *statbuf_get_size(struct stat *sbuf);
static int display(const int mode, const char *path,char* send_buff);

/*主函数*/  
int listDir(char * argv,char * send_buff,char *defaultPath)  
{  
    int i,j;  
    /*mode的前16个位用来标志那些能影响显示的参数，16位之后的位用来标志不影响输出格式的参数。*/  
    int mode=0;  
    char path[LENGTH]={0};/*路径*/  
    int flag=0;/*判断是否指定了目录*/
    memset(path,'\0',sizeof(path));
    /*解析参数*/
    char *tail = NULL;
	char *head = NULL;
	mode=mode|DETAIL|ALL;
    while(1){
		strSplit(&head,&tail,argv," ",0);
		if(head == NULL){
			printf("error");
			return -1;
		}
		if(*head=='-'){/*如果是选项参数*/  
/* 			for(j=1;j<strlen(head);j++){  
				if((head[j]=='l') || (head[j]=='L') )  
					mode=mode|DETAIL;  
				else if(head[j]=='i')  
					mode=mode|INODE;  
				else if(head[j]=='a')  
					mode=mode|ALL;
				else if(head[j]=='A')  
					mode=mode|NOALL;  				
				else{
					printf("no option of %c\n",head[j]); 
				}                     
			}   */
		}else if(*head != ' '){/*参数为目录或文件*/  
			if(flag==1){
				printf("can not specify more then two dir or file");
				return -1;
			}   
			else{
				flag=1; 
			}  			 
			if(*head!='/'){/*相对路径*/  
			    snprintf(path,sizeof(path),"%s/%s",defaultPath,head); 
			}else{/*绝对路径*/  
				strcpy(path,head);  
			}
		} 
		if(tail == NULL){
			break;
		}
		argv = tail;
	}	
  
    if(flag==0){/*未指定任何目录或文件,则使用默认当前目录*/  
		strcpy(path,defaultPath);  
	}
	/*根据mode和路径显示文件*/
	if(display(mode,path,send_buff) == -1){
		return -1;
	}; 	
    return 0;
	
}  

static int display(const int mode, const char *path,char* send_buff)
{
	memset(send_buff,'\0',sizeof(send_buff));
    DIR *dir = opendir(path);
    if(dir == NULL){
		printf("can not open the directory\n");
		return -1;
	}

    struct dirent *dr;
	char filename[LENGTH] = {0};
    while((dr = readdir(dir)))
    {
		memset(filename,'\0',sizeof(filename));
		snprintf(filename,sizeof(filename),"%s/%s",path,dr->d_name);
        //const char *filename = dr->d_name;
		if(!(mode&ALL)){
			if((*dr->d_name) == '.'){
				continue;
			}
		}

        char buf[1024] = {0};
        struct stat sbuf;
        if(lstat(filename, &sbuf) == -1){
			printf("can not get stat %s\n",filename);
			closedir(dir);
			return -1;
		}
        if(mode & DETAIL){
			strcpy(buf, statbuf_get_perms(&sbuf));
			strcat(buf, " ");
			strcat(buf, statbuf_get_user_info(&sbuf));
			strcat(buf, " ");
			strcat(buf, statbuf_get_size(&sbuf));
			strcat(buf, " ");
			strcat(buf, statbuf_get_date(&sbuf));
			strcat(buf, " ");
			if(strlen(path) != 0){
				strcat(buf, statbuf_get_filename(&sbuf, filename) + strlen(path) + 1);
			}else{
				strcat(buf, statbuf_get_filename(&sbuf, filename));
			}
			strcat(send_buff,buf);
			strcat(send_buff,"\r\n");	 
		}else{
			if(strlen(path) != 0){
				strcat(buf, statbuf_get_filename(&sbuf, filename) + strlen(path) + 1);
			}else{
				strcat(buf, statbuf_get_filename(&sbuf, filename));
			}
		    strcat(send_buff,buf);
			strcat(send_buff," ");
		}       
    }
	if(!(mode & DETAIL)){
		strcat(send_buff,"\n");
	}
	//strcat(send_buff,"\r\n");
	//snprintf(send_buff,sizeof(send_buff),"%s\r\n",send_buff);
	printf("%s",send_buff);
    closedir(dir);
    return 0;
}



const char *statbuf_get_perms(struct stat *sbuf)
{
    static char perms[] = "----------";
    mode_t mode = sbuf->st_mode;

    //文件类型
    switch(mode & S_IFMT)
    {
        case S_IFSOCK:
            perms[0] = 's';
            break;
        case S_IFLNK:
            perms[0] = 'l';
            break;
        case S_IFREG:
            perms[0] = '-';
            break;
        case S_IFBLK:
            perms[0] = 'b';
            break;
        case S_IFDIR:
            perms[0] = 'd';
            break;
        case S_IFCHR:
            perms[0] = 'c';
            break;
        case S_IFIFO:
            perms[0] = 'p';
            break;
    }

    //权限
    if(mode & S_IRUSR)
        perms[1] = 'r';
    if(mode & S_IWUSR)
        perms[2] = 'w';
    if(mode & S_IXUSR)
        perms[3] = 'x';
    if(mode & S_IRGRP)
        perms[4] = 'r';
    if(mode & S_IWGRP)
        perms[5] = 'w';
    if(mode & S_IXGRP)
        perms[6] = 'x';
    if(mode & S_IROTH)
        perms[7] = 'r';
    if(mode & S_IWOTH)
        perms[8] = 'w';
    if(mode & S_IXOTH)
        perms[9] = 'x';

    if(mode & S_ISUID)
        perms[3] = (perms[3] == 'x') ? 's' : 'S';
    if(mode & S_ISGID)
        perms[6] = (perms[6] == 'x') ? 's' : 'S';
    if(mode & S_ISVTX)
        perms[9] = (perms[9] == 'x') ? 't' : 'T';

    return perms;
}

const char *statbuf_get_date(struct stat *sbuf)
{
	static char datebuf[64] = {0};
	const char *p_date_format = "%b %e %H:%M";
	struct timeval tv;
	gettimeofday(&tv, NULL);
	time_t local_time = tv.tv_sec;
	if (sbuf->st_mtime > local_time || (local_time - sbuf->st_mtime) > 60*60*24*182)
	{
		p_date_format = "%b %e  %Y";
	}

	struct tm* p_tm = localtime(&local_time);
	strftime(datebuf, sizeof(datebuf), p_date_format, p_tm);

	return datebuf;
}

const char *statbuf_get_filename(struct stat *sbuf, const char *name)
{
    static char filename[1024] = {0};
    //name 处理链接名字
    if(S_ISLNK(sbuf->st_mode))
    {
        char linkfile[1024] = {0};
        if(readlink(name, linkfile, sizeof linkfile) == -1)
            printf("readlink\n");
        snprintf(filename, sizeof filename, " %s -> %s", name, linkfile);
    }else
    {
        strcpy(filename, name);
    }

    return filename;
}

const char *getUserName(uid_t uid)
{
	struct passwd *pw = getpwuid(uid);
	if (pw != NULL)
	{
		return (pw->pw_name);
	}

	return "";
}

const char *getGroupName(gid_t gid)
{
	struct group *g;
    g = getgrgid(gid);
    if (g == NULL) {
        return "";
    }
    return (g->gr_name);
}

const char *statbuf_get_user_info(struct stat *sbuf)
{
    static char info[1024] = {0};
    snprintf(info, sizeof info, " %3d %s %s", sbuf->st_nlink, getUserName(sbuf->st_uid), getUserName(sbuf->st_gid));
    //snprintf(info, sizeof info, " %3d %-8d %-8d ", sbuf->st_nlink, sbuf->st_uid, sbuf->st_gid);
    return info;
}


const char *statbuf_get_size(struct stat *sbuf)
{
    static char buf[100] = {0};
    snprintf(buf, sizeof buf, "%8lu", (unsigned long)sbuf->st_size);
    return buf;
}