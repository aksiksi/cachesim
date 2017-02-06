CC=g++
CFLAGS=-c -std=c++11 -g -Wall
OUT=bin

all: cachesim

cachesim: cachesim.o
	$(CC) $(OUT)/cachesim.o -o cachesim

cachesim.o: src/cachesim.cpp
	$(CC) $(CFLAGS) src/cachesim.cpp -o $(OUT)/cachesim.o

clean:
	rm $(OUT)/*.o
