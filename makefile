lunch:  lunch.o mytime.o
	gcc -lpthread -o lunch lunch.o mytime.o
lunch.o: lunch.c mytime.h
	gcc -c lunch.c -lpthread
mytime.o: mytime.c
	gcc -c mytime.c
