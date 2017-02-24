CC=g++
LFLAGS=-std=c++11 -Wall
CFLAGS=-c -std=c++11 -g -Wall
OBJ=obj
BIN=bin

DEPS=$(OBJ)/util.o $(OBJ)/lru.o $(OBJ)/victim.o $(OBJ)/block.o $(OBJ)/cache.o
CACHESIM=$(BIN)/cachesim
CACHEOPT=$(BIN)/cacheopt

.PHONY: clean

$(OBJ)/%.o: src/%.cpp
	$(CC) $(CFLAGS) $^ -o $@

$(BIN)/%: src/%.cpp $(DEPS)
	$(CC) $(LFLAGS) $^ -o $@

default: $(CACHEOPT) $(CACHESIM)

clean:
	rm -f $(OBJ)/* $(CACHESIM) $(CACHEOPT)
