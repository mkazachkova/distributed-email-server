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

#### Main Program Files
* `net_include.h`: The header file containing structs used by the client and server to pass information amongst and within each other.
* `client.c`: The file containing logic for the client. This class was written to be as lightweight and "dumb" as possible, and queries the server when the user inputs in text.
* `server.c`: The file containing logic for the backend server.
* `makefile`: The makefile to compile all the files.

#### Miscellaneous Files  
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
Updates and email structs each contain `TimeStamp`s: this is a lamport timestamp with an additional `message_index` field to make it generalizable. This will eventually become used in protocols in order to ensure ordering of emails. The struct is declared as below:  

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
* `EmailInfo` contains characters array with text containing contents as specified by the name of the character array.  

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

#### InfoForServer

#### InfoForClient

#### HeaderForCleint

#### MergeMatrix  
Another message unit that is sent between servers during the process of reconciliation is the `MergeMatrix`. This is what is sent whenever a server detects that there is a change in membership. The struct is declared as below:  
```
typedef struct mergematrix {
  int type;                               // 20 is for the matrix
  int machine_index;                      // index from which it came
  int matrix[NUM_SERVERS][NUM_SERVERS];   // the 2-dimensional 5 x 5 reconciliation matrix
} MergeMatrix;
```

* `int type` is the type used to know that this is a `mergematrix` so that we can cast it to the correct type.
* `int machine_index` refers to the index that sent the `mergematrix`.
* `int matrix[NUM_SERVERS][NUM_SERVERS]` is the `mergematrix` itself: a NUM_SERVERS * NUM_SERVERS dimensional matrix.

The `MergeMatrix` is what is used to get servers that were once in different partitions back "up to speed" (so to speak). There will be more discussion of this later in the elaboration of the protocol.


### Important Variables  
Variables that individual clients will contain:
* `int  curr_server = -1;` contains the current server to which a client is connected.
* `char curr_user[MAX_NAME_LEN];` contains the current user that the client is logged in as.
* `char hardcoded_server_names[NUM_SERVERS][MAX_NAME_LEN];` contains the public names of the servers, and is indexed into using `curr_server` when deciding which server to send messages to.
* `char curr_group_connected_to[MAX_NAME_LEN] = "";` contains the name of the current server connected to for the client-server only group (which contains only a single client and server).
* `bool logged_in = false;` contains the state of the user; that is, whether the user is logged in or not.  
* `bool connected_to_server = false;` contains whether the user is connected to a server.
* `bool can_print_user = true;` contains a bool dictating whether we can print `User >` when listing headers.
* `List headers_list;` contains the persistent list of headers, repopulated whenever the person using the client program types in 'l'.


Variables that individual servers will contain:
#### General Core Variables
* `List users_list;` contains [INSERT TEXT HERE]
* `List array_of_updates_list[NUM_SERVERS];` contains [INSERT TEXT HERE]
* `int my_machine_index;` contains [INSERT TEXT HERE]
* `int merge_matrix[NUM_SERVERS][NUM_SERVERS];` contains [INSERT TEXT HERE]
* `int update_index = 0;` contains [INSERT TEXT HERE]
* `bool servers_in_partition[NUM_SERVERS] = { false };` contains [INSERT TEXT HERE]
* `int lamport_counter = 0;` contains [INSERT TEXT HERE]
* `int min_seen_global = -1;` contains [INSERT TEXT HERE]
* `int global_counter = 0;` contains [INSERT TEXT HERE]
* `int min_global = 0;` contains [INSERT TEXT HERE]
* `int num_updates_received = 0;` contains [INSERT TEXT HERE]
* `char sender[MAX_GROUP_NAME];` contains [INSERT TEXT HERE]

#### Global Variables used for Sending Headers to Clients
* `int message_number_stamp = 1;` contains [INSERT TEXT HERE]
* `int num_headers_added;` contains [INSERT TEXT HERE]
* `HeaderForClient *client_header_response;` contains [INSERT TEXT HERE]

#### Fucking Variables for Fucking Flow Control
* `int min_update_global[NUM_SERVERS] = { -1 };` contains [INSERT TEXT HERE] 
* `int max_update_global[NUM_SERVERS] = { -1 };` contains [INSERT TEXT HERE]
* `int num_updates_sent_so_far[NUM_SERVERS] = { 0 };` contains [INSERT TEXT HERE]
* `bool done_sending_for_merge[NUM_SERVERS] = { true };` contains [INSERT TEXT HERE]
* `int current_i = -1;` contains [INSERT TEXT HERE]
* `int num_updates_received_from_myself = 0;` contains [INSERT TEXT HERE]
* `int who_sends[NUM_SERVERS] = { -1 };` contains [INSERT TEXT HERE]
* `int num_sent_in_each_round = 0;` contains [INSERT TEXT HERE]


## System Design  

### Spread Group Architecture  
* There will be a spread group which contains all of the servers; this is the server's "private group".
* There will be a public spread group that each server (and only each individual server) is in; this is the server's "public group" that clients know to contact.
* There will be a spread group containing clients and servers which contains only a single client and a server. This has the current unix timestamp as its name (for guaranteed group name uniqueness) and is only used so clients and servers can tell when a network partition has occurred partitioning a client from the server to which it was once connected.  

### Separation of Clients and Servers  
* We designed the client program to be very much "front-end", providing a GUI for the user and a way to communicate with the servers, but doing nothing "intelligent" on the client side and instead having all memory, ordering, and membership-related tasks delegated to the servers, which are very much the "brain" of the program. There is only one exception to the "memoryless"-ness of the client; this is that the client stores the most recent list of emails that the user has requested to be shown.  

### Generic Linked Lists
* Since much of the program depends on linked lists to remove, insert, and modify messages, we implemented a generic doubly linked list (at the astute suggestion of Emily). The linked list can be found in `generic_linked_list.c`. 

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

The use of the wrapper to contain the list is especially nice, since it keeps the linked list self-contained and abstracted away.

The generic linked list contains function pointers which "outsource" comparisons, printing, etc. to specific node types. There are also several unit tests we wrote to make sure the linked list worked; these can be run by typing in `make test`. The methods that this generic linked list has are listed below:

#### List Manipulation Methods
```
void create_list(List *list, size_t data_size);
void add_to_end(List *list, void *data);
bool remove_from_beginning(List *list, bool (*delete_or_not_func)(void *));
void remove_from_end(List *list);
void insert(List *list, void *data, int (*fptr)(void *, void *));
void* find(List *list, void* data, int (*fptr)(void *, void *));
void* find_backwards(List *list, void* data, int (*fptr)(void *, void *));
bool forward_iterator(List *list, bool (*fptr)(void *));
void backward_iterator(List *list, void (*fptr)(void *));
void empty_list(List *list);
```

#### Accessor Methods
````
void print_list(List *list, void (*fptr)(void *));
void print_list_backwards(List *list, void (*fptr)(void *));
void* get_head(List *list);
void* get_tail(List *list);
```

The names of the methods are quite self-explanatory, and are extensively used to modify lists in both the client and server program.

## Algorithm Description  
### Client-Side
#### User-Input Operations
##### Login as user: `u <username>`
* Retrieve the user name input into the command line.
* Empty the persistent linked list containing headers listed (since that was for the previous user).
* Only if the user is logged in and connected to some server, create an `InfoForServer` request. Populate it with type 1, write the name of the user logged into the client onto the `user_name` field, and send a message to the server to which it is connected.
* Otherwise, if the user was not logged in, then set the `logged_in` field equal to true.

##### Connect to specific mail server: `c <1 - 5>`
* If the user has not been logged in, prompt the user to log in and break.
* Otherwise, parse the input and store in `curr_server`.
* If `curr_group_connected_to` is not the empty string, disconnect from `curr_group_connected_to`, set `curr_group_connected_to` to the empty string, and set `connected_to_server` to false.
* Then, prepare `InfoForServer`, populating it with type 2 and the `curr_user`. Send the server connection request to the server.  
* Begin a timer and enter into a loop.
* While the timer is less than `secs_to_wait` seconds, go through the loop.
  * `SP_poll` on the mailbox.
  * If `SP_poll` returns a value less than 0, print that an error occurred and exit.
  * If `SP_poll` returns a value greater than 0, receive the information using `SP_receive` into an `InfoForClient` object. 
    * If the the thing received was an `Is_caused_leave_mess` message, ignore it.
    * Otherwise, join the group in the `client_server_group_name` field of the `InfoForClient` object that was received. Set the `connected_to_server` and `connection_established` flag to true and break out of the loop.
  * If `SP_poll` returns 0, usleep for 1/10th of a second and enter the loop again.
* If a connection has not yet been established after `secs_to_wait` seconds, print that a server could not be connected to.  

##### List headers of received mail: `l`
* If the user has not yet connected to the server, prompt the user to connect and break.  
* Otherwise, create a `InfoForServer` request, populating it with type 3 and the `curr_user`.  
* Send the request to the server to which it is connected.  
* Empty the persistent `headers_list` and print the User Name and Responding Server to the client, as specified in the design document.  

##### Mail a message: `m`
* If the user has not yet connected to the server, prompt the user to connect and break.  
* Otherwise, get the TO, SUBJECT, and MESSAGE fields from the user.
* Then, create a new `InfoForServer` request, populating it with type 4 and filling in the TO, FROM, SUBJECT, and MESSAGE fields.
* Sent the request to the server to which the client is connected.

##### Delete a message: `d <#>`
* If the user has not yet connected to the server, prompt the user to connect and break.  
* If the amount of nodes in the `headers_list` is 0, print a statement to the user that they cannot delete any messages and break.
* Otherwise, take in the number inputed by the user of the email that the user would like to delete as command-line input.
* If the number they would like to delete is greater than the number of nodes on the `headers_list` or less than 0, print that the number that the user requested is invalid. and break.
* Create a new `InfoForServer` request, populating it with type 5 and the `curr_user`. Set the request's `message_to_delete` field with the lamport timestamp retrieved from the persistent headers list saved when the user typed in 'l'.
* Send the request to the server to which the client is connected.

##### Read a message: `r <#>`
* If the user has not yet connected to the server, prompt the user to connect and break.  
* If the amount of nodes in the `headers_list` is 0, print a statement to the user that they cannot read any messages and break.
* Otherwise, take in the number inputed by the user of the email that the user would like to read as command-line input.
* If the number they would like to read is greater than the number of nodes on the `headers_list` or less than 0, print that the number that the user requested is invalid. and break.
* Create a new `InfoForServer` request, populating it with type 6 and the `curr_user`. Set the request's `message_to_read` field with the lamport timestamp retrieved from the persistent headers list saved when the user typed in 'l'.
* Send the request to the server to which the client is connected.

##### Print membership identities: `v`
* If the user has not yet connected to the server, prompt the user to connect and break.  
* Otherwise, create a new `InfoForServer` request, populating it with type 7 and the `curr_user`. 
* Send the request to the server to which the client is connected.

##### Quit: `q`
* Call the `Bye()` method from `class_user.c` to exit gracefully.

##### Clear: `s`
* Print several new line characters to clear the screen.

##### Undefined Command
* Print the menu again.

#### Information-Receiving Operations
##### Receive Networking-Related Message  
*  
*  

##### Receive "List Headers" message from Server
*  
*  

##### Receive "Read Message" message from Server
*  
*  

##### Receive "Print Membership" message from Server
*  
*  

### Server-Side
#### Client-Server Methods


#### Server-Server Methods


#### Reconciliation


#### Flow Control


## Discussion  


## Final Thoughts  


