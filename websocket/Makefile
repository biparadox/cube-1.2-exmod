FLAGS= -g -o 
CFLAGS= -g -c -DUSER_MODE 
CC=gcc
CCP=g++
FUNCOBJ= websocket_func.o

main: test_main
#policy_test: ui/policy_test.c ui/policy_ui.so
#	$(CC) $(FLAGS) policy_test ui/policy_test.c ui/policy_ui.so
test_main:  test_main.c $(FUNCOBJ) ../lib/libexmodule.a
	$(CC) $(FLAGS) test_main test_main.c $(FUNCOBJ) -L../../cubelib/lib -L../lib -lexmodule -lchannel -lstruct -ljson -lalloc -lstring -lpthread -lwebsockets
websocket_func.o: websocket_func.c websocket_func.h  
	$(CC) $(CFLAGS) websocket_func.c
clean:
	rm -f test_main  *~ *.o *.a
