CC = gcc

CFLAGS = -g -c -pedantic -D_GNU_SOURCE

all: generic_linked_list

generic_linked_list: generic_linked_list.o
	$(CC) -o generic_linked_list generic_linked_list.o

# mcast: mcast.o recv_dbg.o
	# $(CC) -o mcast mcast.o recv_dbg.o

clean:
	rm -f generic_linked_list *.out *.o *~
