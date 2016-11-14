#nakefile
cc = gcc
flag = -lpthread
ftp_client : main.o client.o socket_util.o handles.o file_util.o str_utils.o list_dir.o
	$(cc) main.o client.o socket_util.o handles.o file_util.o str_utils.o list_dir.o $(flag) -o ftp_client

main.o : main.c client.h common.h handles.h
	$(cc) -c main.c $(flag)

client.o : client.c client.h common.h socket_util.h handles.h 
	$(cc) -c client.c $(flag)

handles.o : handles.c handles.h common.h socket_util.h file_util.h str_utils.h ftp_codes.h list_dir.h
	$(cc) -c handles.c $(flag)

socket_util.o : socket_util.c socket_util.h common.h
	$(cc) -c socket_util.c $(flag)

file_util.o : file_util.c file_util.h common.h
	$(cc) -c file_util.c $(flag)

str_utils.o : str_utils.c str_utils.h common.h
	$(cc) -c str_utils.c $(flag)

list_dir.o : list_dir.c list_dir.h str_utils.h
	$(cc) -c list_dir.c $(flag)

clean :
	rm ftp_client *o
