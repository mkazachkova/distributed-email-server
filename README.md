# Final Project: Writing Distributed Software Systems
EN 600.337: Distributed Systems  
Sarah Sukardi (smsukardi@jhu.edu)  
Mariya Kazachkova (mkazach1@jhu.edu)  
December 9, 2016  

## Premise  
We aim to write a fault-tolerant distributed mail service for users over a network of computers that is resilient to network partitions and merges. It provides a client with the ability to connect ot a server, send emails to users, list headers of received mail, read messages, delete messages, and print membership of servers in the server's current network component. 

## Files Included  
#### Linked List Files
* `generic_linked_list.c`: The logic for the generic linked list.
* `generic_linked_list.h`: The generic linked list header file.
* `test_linkedlist.c`: Tests for the basic linked list.
* `test_servermethod.c`: Tests for the linked list on server methods.  

#### Spread Files
* `spread`: The given file for setting up the spread network.
* `spmonitor`: The given file for simulating network partitions and merges.  

### Main Program Files
* `net_include.h`: The header file containing structs used by the client and server to pass information amongst and within each other.
* `client.c`: The file containing logic for the client. This class was written to be as lightweight and "dumb" as possible, and queries the server when the user inputs in text.
* `server.c`: The file containing logic for the backend server.
* `makefile`: The makefile to compile all the files.

#### Miscellaneous  
* `README.pdf`: The design document

## To Run  
* To run the spread network, type in `./spread`.
  * NOTE: This must be done on all 5 machines (ugrad 10-14) before running any clients or servers!
* To run the server, type in `./server [1 - 5]`. 
  * No more input is necessary, but it is vital that all the servers are set up and run before clients begin connecting to it. These mail servers processes are meant to be daemons that run forever.
* To run the client, type in `./client`.
  * A menu will pop up with options to send, read, and delete mail, as well as join other servers and print membership.

## Protocol Overview  

### Data Structures (Structs)
#### TimeStamp
Updates and email structs each contain TimeStamps: this is a lamport timestamp with an additional `message_index` field to make it generalizable. This will eventually become used in protocols in order to ensure ordering of emails. The struct is declared as below:  

```
typedef struct timestamp {
  int         counter;
  int         machine_index;
  int         message_index;
} TimeStamp;
```
* `int counter` is [INSERT TEXT HERE]
* `int machine_index` is [INSERT TEXT HERE]
* `int message_index` is [INSERT TEXT HERE]

#### Email
The most basic data structure we utilize is the `Email`. The struct (and another struct that it itself contains) is declared as below:  
```
typedef struct email {
  EmailInfo         emailInfo;
  bool              read;
  bool              deleted;
  bool              exists;
} Email;
```
* `EmailInfo emailInfo` contains the actual information contained in the email (the sender, recipient, subject, and message).  
* `bool read` contains whether the email has been read or not.
* `bool deleted` contains whether the email has been deleted or not.
* `bool exists` contains whether the email even exists yet or not.
```
typedef struct emailinfo {
  char[MAX_NAME_LEN]  to_field;
  char[MAX_NAME_LEN]  from_field;
  char[MAX_NAME_LEN]  subject;
  char[MAX_MESS_LEN]  message;  
  TimeStamp           timestamp;
} EmailInfo;
```
* EmailInfo contains characters array with text containing contents as specified by the name of the character array.  

The `Email` is never sent by itself, but attached to other data structures that are sent. It contains necessary informational fields about an email as specified in the problem statement: a sender, a recipient, a subject, and a message. We have also given the email a lamport timestamp, to be used in ordering emails, as well as flags, to be used to decide how and whether to display an email to a client when emails are requested to be viewed.    

#### Update
The core message unit that is sent between servers is the `Update`. This is what is sent whenever anything is updated, such as when an email is sent, marked as read, or deleted (this does NOT include changes in grouping; ie. partitions are merges). The `update` struct is declared as below:  
```
typedef struct update {
  int         type;
  TimeStamp   timestamp;
  Email       email;                      // used for new emails
  char        user_name[MAX_NAME_LEN];    // used for new user created
  TimeStamp   timestamp_of_email;         // used for email read or email deleted
  int         index_of_machine_resending_update; //will need to inialize to -1 for all updates when we originally send them 
} Update;
```
* `int type` is an integer used to specify the type of the update. 10 refers to a new email, 11 refers to an email being read, 12 refers to an email being deleted, and 13 refers to a new user being created.
* `TimeStamp timestamp` is the timestamp used to order updates. The `message_index` of the timestamp is used to order updates. 
* `Email email` is filled whenever a new email is created (type 10).
* `char user_name[MAX_NAME_LEN]` is the field containing the name of the user bing created for a type 13 update.
* `TimeStamp timestamp_of_email` is the field containing the lamport timestamp of a particular email being searched for, for a type 11 or 12 update.
* `int index_of_machine_resending_update` is the field containing the index of the machine resending the update, used only during reconciliation.

This `Update` contains a lamport timestamp for ordering purposes (as well as contains an additional field of the server from whence it originated). It also contains an `Email` if a new email has been sent, as well as other potentially useful information depending on the update type. 

* Another message unit that is sent between servers during the process of reconciliation is the `mergematrix`. This is what is sent whenever a server detects that there is a change in membership. The struct is declared as below:  
```
typedef struct mergematrix {
  int type;                               // 20 is for the matrix
  int machine_index;                      // index from which it came
  int matrix[NUM_SERVERS][NUM_SERVERS];   // the 2-dimensional 5 x 5 reconciliation matrix
} MergeMatrix;
```

* `int type` is the type used to know that this is a `mergematrix`.
* `int machine_index` refers to the index that sent the `mergematrix`.
* `int matrix[NUM_SERVERS][NUM_SERVERS]` is the `mergematrix` itself: a NUM_SERVERS * NUM_SERVERS dimensional matrix.

The `MergeMatrix` is what is used to get servers that were once in different partitions back "up to speed" (so to speak).

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

The generic linked list contains function pointers which "outsource" comparisons, printing, etc. to specific node types. There are also several unit tests we wrote to make sure the linked list worked; these can be run by typing in `make test`.  

## Algorithm Description  
### Client-Side

### Server-Side

## Discussion  

## Final Thoughts  

