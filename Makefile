P = urit
CC = gcc
CFLAGS = -Wall -g -O3 --std=c99
OBJECTS = 

$(P): $(OBJECTS)

clean:
	rm -f urit
