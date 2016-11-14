#ifndef STR_UTILS_H
#define STR_UTILS_H
extern void strSplit(char** head,char** tail,char* data,char *delim,int isCpy);
extern void strUpper(char *str);
extern void strLower(char *str);
extern void strTrimCrlf(char *str);
#endif