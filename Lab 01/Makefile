all: zing zing2
	

zing: main.o zing.o
	gcc -o zing main.o zing.o
zing2: main.o zing2.o
	gcc -o zing2 main.o zing2.o

zing2.o: zing2.c
	gcc -Wall -c zing2.c

main.o: main.c
	gcc -Wall -c main.c

clean: 
	rm zing
	rm zing2
