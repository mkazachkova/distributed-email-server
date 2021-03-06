CC = gcc
LD = gcc
CFLAGS = -g -Wall -pedantic -D_GNU_SOURCE
CPPFLAGS=-I. -I/home/cs437/exercises/ex3/include
SP_LIBRARY_DIR=/home/cs437/exercises/ex3

bin: client server

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

client: $(SP_LIBRARY_DIR)/libspread-core.a client.o generic_linked_list.o
	$(LD) -o $@ client.o generic_linked_list.o $(SP_LIBRARY_DIR)/libspread-core.a -ldl -lm -lrt -lnsl $(SP_LIBRARY_DIR)/libspread-util.a

server: $(SP_LIBRARY_DIR)/libspread-core.a server.o generic_linked_list.o
	$(LD) -o $@ server.o generic_linked_list.o $(SP_LIBRARY_DIR)/libspread-core.a -ldl -lm -lrt -lnsl $(SP_LIBRARY_DIR)/libspread-util.a

test: test_linkedlist test_servermethods
	@echo "Running tests..."
	./test_linkedlist
	./test_servermethods
	@echo "All tests passed."

test_linkedlist: test_linkedlist.o generic_linked_list.o
	$(CC) $(CFLAGS) -O -o test_linkedlist test_linkedlist.o generic_linked_list.o

test_linkedlist.o: test_linkedlist.c generic_linked_list.h
	$(CC) $(CFLAGS) -O -c test_linkedlist.c

test_servermethods: test_servermethods.o generic_linked_list.o
	$(CC) $(CFLAGS) -O -o test_servermethods test_servermethods.o generic_linked_list.o

test_servermethods.o: test_servermethods.c generic_linked_list.h
	$(CC) $(CFLAGS) -O -c test_servermethods.c

generic_linked_list.o: generic_linked_list.c generic_linked_list.h
	$(CC) $(CFLAGS) -c generic_linked_list.c

clean:
	rm -f generic_linked_list ./test_linkedlist ./test_servermethods server client *.out *.o *~
