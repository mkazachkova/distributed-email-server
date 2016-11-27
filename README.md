# Final Project: Writing Distributed Software Systems
EN 600.337: Distributed Systems  
Sarah Sukardi (smsukardi@jhu.edu)  
Mariya Kazachkova (mkazach1@jhu.edu)  
December 9, 2016  

## Premise  
We aim to write a fault-tolerant distributed mail service for users over a network of computers that is resilient to network partitions and merges. It provides a client with the ability to connect ot a server, send emails to users, list headers of received mail, read messages, delete messages, and print membership of servers in the server's current network component.  

## Files Included  
*  
*  
*  

## To Run  

## Overview  

### Data Structures (Structs)
* The most basic data structure we utilize is the `Email`. The struct (and another struct that it itself contains) is declared as below:  
```
typedef struct email {
  EmailInfo           emailInfo;
  bool                read;
  bool                deleted;
} Email;
```

```
typedef struct emailinfo {
  char[MAX_NAME_LEN]  to_field;
  char[MAX_NAME_LEN]  from_field;
  char[MAX_NAME_LEN]  subject;
  char[MAX_MESS_LEN]  message;  
  TimeStamp           timestamp;
} EmailInfo;
```

The `Email` is never sent by itself, but attached to other data structures that are sent; ie. attached to an update of type `1`. It contains necessary informational fields about an email as specified in the problem statement.  

* The core message unit that is sent between servers is the `Update`. This is what is sent whenever anything is updated, such as when an email is sent, marked as read, or deleted (this does NOT include changes in grouping; ie. partitions are merges). The `update` struct is declared as below:  
```
typedef struct update {

  int                 type;               //1: new email. 2: email read. 3: email deleted. 4: new user created.
  TimeStamp           timestamp;
  Email               email;              //used for new emails
  char[MAX_NAME_LEN]  user_name;          //used for new user created
  TimeStamp           timestamp_of_email; //used for email read or email deleted
  int[NUM_SERVERS]    updates_array;
} Update;
```

This `Update` contains a lamport timestamp for ordering purposes (as well as contains an additional field of the server from whence it originated). It also contains an `Email` if a new email has been sent, as well as other potentially useful information depending on the update type. Most notably, it contains an `updates_array` of integers, where it periodically signals what it has received up to from every server. This is attached to every update and not just sent during partitions as the occurrence of a partition/merge is unpredictable and thus it is safest to send this `updates_array` whenever there is an update. The array will be used to update each server's 5 X 5 matrix (elaborated later in the discussion of Other Important Variables) and eventually used during merges and for eventual path propagation.  

* Finally, the last message unit that is sent between servers is the `mergematrix`. This is what is sent whenever a server detects that there is a change in membership. The struct is declared as below:  
```
typedef struct mergematrix {
  int machine_index;                      //index from which it came
  int[NUM_SERVERS][NUM_SERVERS] matrix;   //the 2-dimensional 5 x 5 reconciliation matrix
} MergeMatrix;
```

The machine index from which the server originated is sent. The important piece of information is the `matrix`; that is, the 2-dimensional 5 x 5 reconciliation matrix. This is what is used to get servers that were once in different partitions back "up to speed" (so to speak).

### Other Important Variables
Variables that individual servers will contain:
*   
*  

## System Design  

### Spread Group Architecture
* There will be a spread group which contains all of the servers.
* There will be a spread group which contains all clients that are logged into a particular server, as well as the server to which they are connected.

### Separation of Clients and Servers
* We designed the client program to be very much "front-end", providing a GUI for the user and a way to communicate with the servers, but doing nothing "intelligent" on the client side and instead having all memory, ordering, and membership-related tasks delegated to the servers, which are very much the "brain" of the program.  

### Generic Linked Lists
* Since much of the program depends on linked lists to remove, insert, and modify messages, we implemented a generic doubly linked list (at the suggestion of Emily). The linked list can be found in `generic_linked_list.c`. 

The core unit of the linked list, of course, is the node:  
```
typedef struct node {
  void* data;
  struct node *next;
  struct node *prev;
} Node;
```

And we also implemented a wrapper since it was doubly-linked:  
```
typedef struct {
  Node *head;
  Node *tail;
  int num_nodes;
  int node_size;
} List;
```

The generic linked list contains function pointers which "outsource" comparisons, printing, etc. to specific node types.  

## Algorithm Description  
### Client-Side

### Server-Side

## Discussion  

## Final Thoughts  

