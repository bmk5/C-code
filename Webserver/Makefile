all:  libs webserver object 

libs : lib.c lib.h
	gcc -Wall -c lib.c

webserver : webserver.c lib.h
	gcc -Wall -pthread -c webserver.c 

object: lib.o webserver.o
	gcc -Wall -pthread -o webserver webserver.o lib.o

clean :
	rm *.o
	rm webserver
