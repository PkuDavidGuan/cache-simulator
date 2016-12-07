CC=g++

all: sim

sim: main_fy.o cache.o memory.o replacealgorithm.o
	$(CC) -g -mcmodel=large -o $@ $^

main_fy.o: cache.h
	$(CC) -g -mcmodel=large -c main_fy.cc

cache.o: cache.h def.h
	$(CC) -g -mcmodel=large -c cache.cc

memory.o: memory.h
	$(CC) -g -mcmodel=large -c memory.cc

replacealgorithm.o: cache.h def.h
	$(CC) -g -mcmodel=large -c replacealgorithm.cc

.PHONY: clean
clean:
	rm -rf sim *.o
