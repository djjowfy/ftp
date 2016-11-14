#include "common.h"
#include "str_utils.h"

void strSplit(char** head,char** tail,char* data,char *delim,int isCpy){
	if(isCpy){
		if(strstr(data,delim) == NULL){
			strcpy(*head,data);
			return;
		}
		char * left;
		char * right;
		left = strtok_r(data,delim,&right);
		strcpy(*head,left);
		while((*right) == ' ')right ++;
		strcpy(*tail,right);
	}else{
		if(strstr(data,delim) == NULL){
			*head = data;
			*tail = NULL;
			return;
		}
		*head = strtok_r(data,delim,tail);	
	}

}

void strUpper(char *str)
{
	int i,count = strlen(str);
	for(i = 0;i < count;i ++)
	{
		*str = toupper(*str);
		str++;
	}
}

void strLower(char *str)
{
	int i,count = strlen(str);
	for(i = 0;i < count;i ++)
	{
		*str = tolower(*str);
		str++;
	}
}

void strTrimCrlf(char *str)
{
	char *p = &str[strlen(str)-1];
	while (*p == '\r' || *p == '\n')
		*p-- = '\0';

}



