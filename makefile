CC = gcc
CFLAGS = -g  -pedantic -D_GNU_SOURCE

bin: test

test: test.o generic_linked_list.o
	$(CC) $(CFLAGS) -O -o test test.o generic_linked_list.o

test.o: test.c generic_linked_list.h
	$(CC) $(CFLAGS) -O -c test.c

generic_linked_list.o: generic_linked_list.c generic_linked_list.h
	$(CC) $(CFLAGS) -c generic_linked_list.c

# mcast: mcast.o recv_dbg.o
# $(CC) -o mcast mcast.o recv_dbg.o

clean:
	rm -f generic_linked_list *.out *.o *~
