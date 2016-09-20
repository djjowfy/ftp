#nakefile
cc = gcc
flag = -lpthread
ftp_client : main.o client.o socket_util.o handles.o  file_util.o
	$(cc) main.o client.o handles.o socket_util.o  file_util.o  $(flag) -o ftp_client

main.o : main.c client.h common.h
	$(cc) -c main.c $(flag)

client.o : client.c client.h common.h handles.h socket_util.h
	$(cc) -c client.c $(flag)

handles.o : handles.c handles.h common.h file_util.h
	$(cc) -c handles.c $(flag)

socket_util.o : socket_util.c socket_util.h common.h
	$(cc) -c socket_util.c $(flag)

file_util.o : file_util.c file_util.h common.h
	$(cc) -c file_util.c $(flag)
clean :
	rm ftp_client *o

