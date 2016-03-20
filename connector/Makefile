FLAGS= -g -o 
CFLAGS= -g -c -DUSER_MODE 
CC=gcc
CCP=g++
FUNCOBJ= connector.o connector_unix.o connector_inet.o connector_hub.o

main: ../lib/libconnector.a test_srv  test_client test_client1
#policy_test: ui/policy_test.c ui/policy_ui.so
#	$(CC) $(FLAGS) policy_test ui/policy_test.c ui/policy_ui.so
test_srv:  test_srv.c $(FUNC_OBJ) ../../cubelib/lib/libstruct.a ../lib/libconnector.a
	$(CC) $(FLAGS) test_srv test_srv.c -L../../cubelib/lib -L../lib -lstruct -lconnector
test_client1:  test_client1.c $(FUNC_OBJ) ../lib/libconnector.a ../../cubelib/lib/libstruct.a
	$(CC) $(FLAGS) test_client1 test_client1.c -L../../cubelib/lib -L../lib -lstruct -lconnector
test_client:  test_client.c $(FUNC_OBJ) ../lib/libconnector.a ../../cubelib/lib/libstruct.a
	$(CC) $(FLAGS) test_client test_client.c -L../../cubelib/lib -L../lib -lstruct -lconnector
../lib/libconnector.a:  $(FUNCOBJ)
	ar cr libconnector.a $(FUNCOBJ) 
	mv    libconnector.a ../lib
connector.o: connector.c  
	$(CC) $(CFLAGS) connector.c
connector_unix.o: connector_unix.c  
	$(CC) $(CFLAGS) connector_unix.c
connector_inet.o: connector_inet.c  
	$(CC) $(CFLAGS) connector_inet.c
connector_hub.o: connector_hub.c  
	$(CC) $(CFLAGS) connector_hub.c
clean:
	rm -f test_srv test_client test_client1  *~ *.o *.a
