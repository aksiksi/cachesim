CC=g++
CFLAGS=-c -std=c++11 -g -Wall
OUT=bin

all: cachesim

cachesim: cachesim.o cache.o
	$(CC) $(OUT)/*.o -o cachesim

cachesim.o: src/cachesim.cpp
	$(CC) $(CFLAGS) src/cachesim.cpp -o $(OUT)/cachesim.o

cache.o: src/cache.cpp
	$(CC) $(CFLAGS) src/cache.cpp -o $(OUT)/cache.o

clean:
	rm $(OUT)/*.o
