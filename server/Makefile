#nakefile
cc = gcc
flag = -lpthread
ftp_server : main.o server.o socket_util.o handles.o file_util.o
	$(cc) main.o server.o socket_util.o handles.o file_util.o $(flag) -o ftp_server

main.o : main.c server.h common.h handles.h
	$(cc) -c main.c $(flag)

server.o : server.c server.h common.h socket_util.h handles.h 
	$(cc) -c server.c $(flag)

handles.o : handles.c handles.h common.h socket_util.h file_util.o
	$(cc) -c handles.c $(flag)

socket_util.o : socket_util.c socket_util.h common.h
	$(cc) -c socket_util.c $(flag)
file_util.o : file_util.c file_util.h common.h
	$(cc) -c file_util.c $(flag)
clean :
	rm ftp_server *o

