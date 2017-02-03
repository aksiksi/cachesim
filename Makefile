CC=g++
CFLAGS=-c -g -Wall
OUT=bin

cachesim: cachesim.o
	$(CC) $(OUT)/cachesim.o -o cachesim

cachesim.o: src/cachesim.cpp
	$(CC) $(CFLAGS) src/cachesim.cpp -o $(OUT)/cachesim.o

clean:
	rm $(OUT)/*.o
