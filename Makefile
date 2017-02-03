CC=g++
CFLAGS=-c -g -Wall
OUT=bin

cachesim: main.o
	$(CC) $(OUT)/main.o -o cachesim

main.o: src/main.cpp
	$(CC) $(CFLAGS) src/main.cpp -o $(OUT)/main.o

clean:
	rm $(OUT)/*.o
