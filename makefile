CC = gcc
LD = gcc
CFLAGS = -g -Wall -pedantic -D_GNU_SOURCE
CPPFLAGS=-I. -I/home/cs437/exercises/ex3/include
SP_LIBRARY_DIR=/home/cs437/exercises/ex3

bin: server client

test: test_linkedlist
	@echo "Running test..."
	./test_linkedlist
	@echo "All tests passed."

test_linkedlist: test.o generic_linked_list.o
	$(CC) $(CFLAGS) -O -o test test.o generic_linked_list.o

test.o: test.c generic_linked_list.h
	$(CC) $(CFLAGS) -O -c test.c

generic_linked_list.o: generic_linked_list.c generic_linked_list.h
	$(CC) $(CFLAGS) -c generic_linked_list.c

# mcast: mcast.o recv_dbg.o
# $(CC) -o mcast mcast.o recv_dbg.o

clean:
	rm -f generic_linked_list server client *.out *.o *~
