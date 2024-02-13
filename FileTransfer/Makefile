all:receiver sender 

# libs : lib.c lib.h
# 	gcc -Wall lib.c

receiver: receiver.c 
	gcc -Wall receiver.c lib.c -o receiver -pthread -g

sender: sender.c 
	gcc -Wall sender.c lib.c -o sender -pthread -g


# object1: receiver.o lib.o
# 	gcc -Wall -pthread -o receiver receiver.o lib.o

# object2: sender.o lib.o
# 	gcc -Wall -pthread -o sender sender.o lib.o

# object3: rec2.o lib.o
# 	gcc -Wall -pthread -o rec2 rec2.o lib.o

clean :
	rm receiver
	rm sender
	rm */result.txt
