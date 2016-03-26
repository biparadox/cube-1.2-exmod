FLAGS= -g -o 
CFLAGS= -g -c -DUSER_MODE 
CC=gcc
CCP=g++
FUNCOBJ= ex_module.o

main: ../lib/libexmodule.a test_main
#policy_test: ui/policy_test.c ui/policy_ui.so
#	$(CC) $(FLAGS) policy_test ui/policy_test.c ui/policy_ui.so
test_main:  test_main.c $(FUNC_OBJ) ../../cubelib/lib/libstruct.a ../lib/libexmodule.a
	$(CC) $(FLAGS) test_main test_main.c -L../../cubelib/lib -L../lib -lexmodule -lstruct -ljson -lalloc -lstring -lpthread
../lib/libexmodule.a:  $(FUNCOBJ)
	ar cr libexmodule.a $(FUNCOBJ) 
	mv    libexmodule.a ../lib
ex_module.o: ex_module.c  
	$(CC) $(CFLAGS) ex_module.c
clean:
	rm -f test_main  *~ *.o *.a
