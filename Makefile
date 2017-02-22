CC=g++
CFLAGS=-c -std=c++11 -g -Wall

all: cachesim clean

cachesim: lru.o victim.o block.o cachesim.o cache.o
	$(CC) *.o -o cachesim

cachesim.o: src/cachesim.cpp
	$(CC) $(CFLAGS) src/cachesim.cpp -o cachesim.o

cache.o: src/cache.cpp
	$(CC) $(CFLAGS) src/cache.cpp -o cache.o

block.o: src/block.cpp
	$(CC) $(CFLAGS) src/block.cpp -o block.o

victim.o: src/victim.cpp
	$(CC) $(CFLAGS) src/victim.cpp -o victim.o

lru.o: src/lru.cpp
	$(CC) $(CFLAGS) src/lru.cpp -o lru.o

clean:
	rm -f *.o
