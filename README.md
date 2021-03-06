# Final Project: Writing Distributed Software Systems
EN 600.337: Distributed Systems  
Sarah Sukardi (smsukardi@jhu.edu)  
Mariya Kazachkova (mkazach1@jhu.edu)  
December 10, 2016  

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
  * Make sure that all the servers run are given UNIQUE names. We suggest running server 1 on ugrad10, server 2 on ugrad11, etc up to server 5 on ugrad14.
* To run the client, type in `./client` on any of the machines running Spread.
  * A menu will pop up with options to send, read, and delete mail, as well as join other servers and print membership.
* To run the partition/merge simulator, type in `./spmonitor`.
  * A menu will pop up with options to create network partitions.

## Protocol Overview 

### Brief Summary
We implemented a server and client program to run on 5 servers. In order to reconcile servers during network partitions and merges, we utilized the anti-entropy protocol. Each server kept a "merge matrix" of what they knew that servers had from other servers. Upon the event of a merge, these "merge matrices" would be sent around. The maximum value of all merge matrices received would be copied into the server's updating merge matrix. Then, the maximum of all the elements in the new partition's column would become the new maximum value in each column for the servers in the partition. Finding the difference between the minimum value in each column for servers in a new partition and the maximum value would allow each process to know if it is responsible for resending updates and which updates it must resend. If there is a tie for which process knows the most about another process (both processes being in the same partition), then the process with the lower machine index will resend. By this protocol we would be able to implement server resiliency to partitions and merges.  

This high-level description will be elaborated more in the Algorithm section of the design document, particularly the reconciliation section. The section will also talk about how we integrate flow control, cascading merges, and garbage collection for updates.  

### Data Structures (Structs)
#### TimeStamp
Updates and email structs each contain `TimeStamp`s: this is a lamport timestamp with an additional `message_index` field to make it generalizable to both structs. The `TimeStamp` is used in protocols in order to ensure ordering of emails and updates. The struct is declared as below:  

```
typedef struct timestamp {
  int         counter;
  int         machine_index;
  int         message_index;
} TimeStamp;
```
* `int counter` is the lamport time stamp counter; it is incremented every time an email is sent. This is used to order emails.
* `int machine_index` is the machine index from which the email originated. It is the second part of the lamport time stamp. It is used in conjunction with `counter` to order emails.
* `int message_index` is the index of the message, used for ordering updates. It is incremented every time an update is sent.

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
The core message unit that is sent between servers is the `Update`. This is what is sent whenever anything is updated, such as when an email is sent, marked as read, or deleted. The `Update` struct is declared as below:  
```
typedef struct update {
  int         type;
  TimeStamp   timestamp;
  Email       email;
  char        user_name[MAX_NAME_LEN];
  TimeStamp   timestamp_of_email;
  int         index_of_machine_resending_update;
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
The message unit for messages that are sent from clients to servers. This is what is sent whenever the user inputs anything into the client program. The program parses the user input and then packages it into an `InfoForServer` object to request the necessary information from the user. The `InfoForServer` struct is declared as below:
```
typedef struct info_for_server {
  int       type;
  char      user_name[MAX_NAME_LEN];
  Email     email;
  TimeStamp message_to_read;
  TimeStamp message_to_delete;
} InfoForServer;
```

* `int type` is an integer that defines how the client will process the `InfoForServer` object. 1 refers to a change of user, 2 refers to a request to connect to the server, 3 refers to a request to list headers, 4 refers to a request to mail a message to another user, 5 refers to a delete message request, 6 refers to a read message request, and 7 refers to a print membership request.
* `char user_name` contains the name of the user who sent the request.
* `Email email` contains the email that the user wants to send to another user, for a request of type 4.
* `TimeStamp message_to_read` contains the lamport timestamp of the email to read, for a request of type 6.
* `TimeStamp message_to_delete` contains the lamport timestamp of the email to delete, for a request of type 5.

#### InfoForClient
The message unit for messages that are sent from servers to clients. This is what is sent from the server back to the client so that that client can print things onto the screen for the person using the client program. Thi includes headers, messages to be read, and membership messages that tell who else is in the client's current partition. The `InfoForClient` struct is declared as below:

```
typedef struct info_for_client {
  int    type; 
  Email  email;     
  char   memb_identities[NUM_SERVERS][MAX_NAME_LEN]; 
  char   client_server_group_name[MAX_NAME_LEN];
} InfoForClient;
```
* `int type` is an integer that defines how the server will process the `InfoForClient` object. 2 refers to a message containing an email body that the client has requested, 3 refers to memberships that a client has requested, and 4 refers to the confirmation from a server that it has successfully connected to a client.
* `Email email` contains the contents of the email that a client has requested.
* `char memb_identities` contains the membership identities, responding to when a client requests memberships.
* char client_server_group_name` contains the name of the group to which the client is to connect when it successfully connects to a server.  

#### HeaderForClient
A specialized message unit containing only headers for a client to print out to a user. This has been separated from the `InfoForClient` object as to stay within the limits of the size of a packet of under 1600 bytes. The `HeaderForClient` struct is declared as below:

```
typedef struct header_for_client {
  int    type; 
  Header headers[MAX_HEADERS_IN_PACKET];
  bool   done;
} HeaderForClient;
```

The HeaderForClient object contains a type of 1 to identify it when the client is processing things that were sent to it. It contains an array of headers, as well as a boolean for determining when the server has finished sending its last header packet. The individual header struct is declared as below:

```
typedef struct header {
  int       message_number;                 
  char      sender[MAX_NAME_LEN];
  char      subject[MAX_NAME_LEN];
  bool      read;
  TimeStamp timestamp;
} Header;
```
* `int message_number` is the message number of the message that was sent.
* `char sender` is who sent the message.
* `char subject` is the subject of the message.
* `bool read` contains whether the message was read or not.
* `TimeStamp timestamp` is the lamport timestamp of the email, used to uniquely identify the email.

#### MergeMatrix  
Another message unit that is sent between servers during the process of reconciliation is the `MergeMatrix`. This is what is sent whenever a server detects that there is a change in membership. The struct is declared as below:  
```
typedef struct mergematrix {
  int type;
  int machine_index;
  int matrix[NUM_SERVERS][NUM_SERVERS];
} MergeMatrix;
```

* `int type` is the type (type 20) used to know that this is a `mergematrix` so that we can cast it to the correct type.
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
* `List users_list;` contains a linked list of all the users on the server.
* `List array_of_updates_list[NUM_SERVERS];` contains an array of linked lists, where each linked lists contains the updates associated with a particular server.
* `int my_machine_index;` contains what the server's machine index is, as taken from command-line input.
* `int merge_matrix[NUM_SERVERS][NUM_SERVERS];` contains the server's merge matrix, used in the reconciliation process.
* `int update_index = 0;` contains the number of updates (each individual server's update counter); it is incremented by 1 every time a server sends a new update.
* `bool servers_in_partition[NUM_SERVERS] = { false };` contains which servers are in the server's partition.
* `int lamport_counter = 0;` contains the lamport time stamp counter.
* `int min_global = 0;` contains the message_index that e can delete up to while deleting.  
* `int num_updates_received = 0;` contains the number of updates that a process has received in order to check when to check if can delete  
* `char sender[MAX_GROUP_NAME];` contains the name of who sent the message; populated during SP_receive.  

#### Global Variables used for Sending Headers to Clients
* `int message_number_stamp = 1;` contains  number used and incremented when displaying emails to the user.    
* `int num_headers_added;` contains the number of Header structs to have been added to a HeaderForClient struct.  
* `HeaderForClient *client_header_response;` contains a global HeaderForClient struct that will be populated when the client as to list his or her inbox.  

#### Variables for Flow Control
* `int min_update_global[NUM_SERVERS] = { -1 };` contains the minimum value seen in each column (representing where we need to start sending from during reconciliation).     
* `int max_update_global[NUM_SERVERS] = { -1 };` contains the message index of thet process's last update for a particular index (representing which update to send up until during reconciliation).   
* `int num_updates_sent_so_far[NUM_SERVERS] = { 0 };` contains a number representing how many updates a process has sent so far from each of its linked lists of updates (one for every process).   
* `bool done_sending_for_merge[NUM_SERVERS] = { true };` contains whether or not the process has completed sending all of the updates that it needed to resend during reconciliation.  
* `int current_i = -1;` contains a number so that a particular index can be used outside of the scope of the main method.   
* `int num_updates_received_from_myself = 0;` contains the number of updates that a processes has received that that same process had resent during reconciliation.  
* `int who_sends[NUM_SERVERS] = { -1 };` contains a number representing which process is responsible for sending the updates for the process associated with that particular index.   
* `int num_sent_in_each_round = 0;` contains the number of total updates that a process has resent during each round of sending while reconciling (used to know how many updates must be received before sending again).   


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

The use of the wrapper to represent the list in our main code is especially nice, since it keeps the linked list self-contained and abstracted away.  

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
````

The names of the methods are quite self-explanatory as to explaining the functions of the methods. These methods are extensively used to modify lists in both the client and server program and their generic structure is integral to keeping the program concise, maintainable, and less prone to error.  

## Algorithm Description  
### Client-Side
We use event-driven programming on both the client and server-side. We wrote a method that receives and parses input (`Read_message`). We also have a method that takes in user input (`User_command`). Both of these are `E_attach`ed and then `E_handle_event`s-ed; that is, triggered when either something is received or the user types in something. The User-Input Operations section talks about how clients handle user input, and the Information-Receiving Operations section talks about how clients handle messages received.

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
* If it is a transition, join, or leave message, print `User >` and return.
* If it is a network message, leave the group stored in `curr_group_connected_to` (the client-server spread group must have partitioned, where the client was partitioned from the server to which it was connected), set `connected_to_server` to false, print `User >`, and return.

##### Receive "List Headers" message from Server
* Process the first one received. Print the headers and save the header printed to the persistent linked list.
* While there remain headers to be received, keep processing them, printing them, and saving them to the persistent linked list.

##### Receive "Read Message" message from Server
* If the email has been deleted, print that the user cannot view the email. 
* Otherwise, print the contents of the email.  

##### Receive "Print Membership" message from Server
* Look through the `memb_identities` array of "strings" (char arrays). If it is not empty string, print the membership.

### Server-Side
We use event-driven programming on the server-side. We only have one method that is `E_attach`ed and then `E_handle_event`s-ed that parses messages received, as there is no user input on the server-side.  

#### Client-Server Methods
These are the methods that directly process messages that are from clients and send messages to all the servers in the spread group.

##### "Create New User" Request
* Cast the message into an `InfoForServer` object.
* If the user sent in the request is nonexistent, create the user.
* If a new user was created, create an `Update` object with type 13, populating the timestamp of the UPDATE with the `update_index` (which is always incremented by 1 beforehand), the sending server's `machine_index`, and the `user_name`. 
* Send the `Update` to all other servers in the server's private group.

##### "New Connection" Request
* Cast the message into an `InfoForServer` object.
* If the user sent in the request is nonexistent, create the user.
* Send an `InfoForClient` object back to the client with the group name that the server joined, for the single client-server group with group name retrieved from the current time. Multicast this message back to the client.
* If a new user was created, create an `Update` object with type 13, populating the timestamp of the UPDATE with the `update_index` (which is always incremented by 1 beforehand), the sending server's `machine_index`, and the `user_name`. 
* Send the `Update` to all other servers in the server's private group.

##### "List Headers" Request
* Cast the message into an `InfoForServer` object.
* Find the user in the `users_list` of the username of the client that sent the request.
* In a `InfoForClient` object, send all the headers that are NOT (deleted or nonexistent) in the `email_list` associated with the user we just found, iterating backwards and stamping them with a message number and the lamport time stamp of the EMAIL. Send them in packets containing `MAX_HEADERS_IN_PACKET` headers, a value that was optimized to fill up (not overfill) a single packet.
* Send the very last `InfoForClient` object with headers back to the client.

##### "Mail Message" Request
* Cast the message into an `InfoForServer` object.
* In an `Update`, send the email, populating the timestamp of the UPDATE with the `update_index` (which is always incremented by 1 beforehand), the sending server's `machine_index`, and populating the timestamp of the EMAIL with the `lamport_counter` (which is always incremented by 1 beforehand) and the sending server's `machine_index`. Set the `exists`, `deleted`, and `read` fields on the email to `true`, `false`, and `false` respectively.
* Send the `Update` to all other servers in the server's private group.

##### "Read Message" Request
* Cast the message into an `InfoForServer` object.
* In an `Update`, send the request, populating the timestamp of the update as done above and the `timestamp_of_email` with the timestamp sent in the `InfoForServer`'s `message_to_read` field.
* Send the `Update` to all other servers in the server's private group.
* Find the user associated with the client who sent the message's `user_name` and find the actual email requested by the lamport time stamp as the comparator.
* Send the found email back to the client.  

##### "Delete Message" Request
* Cast the message into an `InfoForServer` object.
* In an `Update`, send the request, populating the timestamp of the update as done above and the `timestamp_of_email` with the timestamp sent in the `InfoForServer`'s `message_to_delete` field.
* Send the `Update` to all other servers in the server's private group.

##### "Print Membership" Request
* Cast the message into an `InfoForServer` object.
* Look at the `servers_in_partition` boolean array. For every server that exists, populate the `memb_identities` in an `InfoForClient` object at that index with the name of the server; otherwise populate it with the empty string.
* Send the memberships back to the client.

#### Server-Server Methods
These are the methods that process messages from other servers in the partition that the server is in.

##### "New Email" Update
* Cast the message into an `Update` object.
* Create the user if it does not exist from the `Update`'s `email`'s `to_field`.
* Update our own lamport counter to be the max of our existing `lamport_counter` and the `lamport_counter` of the email we just received.
* Update the server's own `merge_matrix` with the `message_index` of the message just received (the column of every server in the current server's partition).
* Look for if the update already exists: if it does, ignore it and return.
* Look for the user in the `to_field` of the email from the `Update`.
* Look for the email for the user that we found. If the email already exists, return.
* If the email does not exist, insert the received `Update` into the Updates linked list for the update associated with our server and insert the received `Email` into the email linked list in teh user that was found previously.

##### "Read Email" Update
* Cast the message into an `Update` object.
* Update the merge matrix as described in the "New Email" Update.
* Look for the update. If it already exists, return. 
* Otherwise, insert the `Update`. Then create the user in the `user_name` field if it doesn't exist yet. Then look for the email in that user. If it doesn't exist, insert the `new_email` field with the `read` flag set to true in that user's email linked list.


##### "Delete Email" Update
* "Delete Email" proceeds similarly as "Read Email", but setting the `deleted` flag to true rather than the `read` flag.

##### "New User" Update
* Cast the message into an `Update` object.
* Create the user from the update's `user_name` field if it doesn't exist yet.
* Update the merge matrix as described previously.
* Look for the update. If it already exists, return.
* Otherwise, inser the `Update`.

#### Reconciliation (Using Anti-Entropy)
Our reconciliation process starts when we receive a `Caused_By_Network` message. This message means that a message has been sent under the instruction of `spmonitor` (so memberships have changed, meaning it is time to try reconciling). The first thing we do is check if the message was caused by a Server-Client group being broken up due to the partition. If this is the case, the server leaves the Server-Client group and we break.  

If the message was not dealing with a Server-Client group, the first thing we do is set everything in our `servers_in_partition` array to false. The next step we do deals with taking care of cascading merges, so it will be discussed in the section below. We now look at the `target_groups` array (which was populated when we did `SP_receive` to get the network message) to see which servers are in our partition (we set the locations in our `servers_in_array` array corresponding to these servers to true). Now we know which servers are in our new partition. 

Next, we take our local copy of our merge matrix and copy it into a `MergeMatrix` struct. We set the type on this struct to 20 (which represents a merge matrix) and set the machine index field to our own machine index. Then, we send out this `MergeMatrix` struct. 

Next, we wait to receive `number_of_groups` matrices (which is set to the number of groups received during `SP_receive`). For each one of these matrices we process the received merge matrix by doing two things: for each cell we take the minimum of the cell in our merge matrix and of the one we just received and set the corresponding cell in our `min_array` to that value; we also that the maximum of the cell in our matrix and of the matrix received and set the current cell being processeed to that value. 

We are now able to start figuring out who will send the updates. We loop through each column in our (now updated) merge matrix and figure out which of the processes in our partition has the maximum value for that column (this is the process responsible for sending the updates associated with the particular process that corresponds tho that column). We then set that location in our `who_sends` array to the value of the machine responsible for doing the sending.  

Now we come to the actual sending part of the reconciliation process. In the most basic sense, we loop through the `who_sends` array and if the value stored in there matches `my_machine index` then that specicic proccess moves on to send all of the updates from the minimum value for that column in the `min_array` up until the most recent updated it received. The actual details of this sending will be discussed in depth in the Flow Control section below.


#### Cascading Merges  
As described in class, a cascading merge is when a merge/parititon occur while processes are trying to reconcile. While this is very difficult to test (also stated in class), we included logic for it in our code that we believe should take of the issue. Prior to starting to reconcile a process must update its corresponding row in its own merge matrix (this is prior to sending the merge matrices out to everyone in the partition). A process does this by looping through its row in the merge matrix and setting each cell to the message index of the last update in each index of its `array_of_updates_list`. This means that when the merge matrix is sent out to the other processes in the partition, each process will know exactly up to which update the other processes has. Thus, when it comes time for updates to be resent each process has an accurate view of what other processes in its partition both have and do not have, meaing that the right updates can be sent out even if a merge/partition occurs while reconciliation is occurring.  

#### Flow Control  
Flow control is needed when servers are resending updates during the reconciliation process (the could have millions of messages to resend). Under Tom's advising we decided to do flow control in a way that was similar to the way flow control was done in assignment three. The basis of this flow control involves sending x messages and then waiting to receive x messages before sending another x messages. However, the flow control here becomes significantly more complicated because a single process could be responsible for sending updates from more than one process (meaning process a might need to send a's, c's, and d's updates).

Because we implemented our storage of updates using generic linked lists we needed to store numerous global variables in order to make the flow control work. For each list of updates in the `array_of_updates_list` we needed to store a starting point (the min for the corresponding column in `min_array`), an ending point (the message index of the last update in the list corresponding with that column), a number representing now many updates have been sent from that particular linked list, and a boolean value declaring whether we are done sending for that linked list or not. Our algorithm works as follows: 

* For each index that a particular process is responsible for sending for:  
* Find the minimum and the maximum value. Store both of these in the coreesponding global arrays (see variables section above) 
* Begin to send. Before you send an update stamp your machine index in the `index_of_machine_resending_update` field. Once you send an update increment a counter representing how many updates you have sent from that particular list. Also increment a counter indicating how many updates you have sent total (from all linked lists) in that round of sending. Furthermore, increment the variable indicating the update where you start sending from (this will help us know when to stop sending)  
* If the number you are starting from equals the number that you are ending at then this means that you are done sending. The boolean representing whether sending is still occurring for that particular linked list will be set to false  
* If you hit that you have sent 10 updates from that particular linked list prior to hitting the above condition then break   out of the sending section and set the boolean representing whether you are still sending for that linked list to true   
* In each round of sending do the above for each index in the who_sends array  
* After a round of sending now we process updates. As you are processesing updates check the `index_of_machine_resending_update` field. If this number corresponds to your machine index the increment a counter the represents the number of resent updates you have received from yourself  
* Once your counter of the number of updates that you received that were the ones that you resent is equivalent to the number of updates that you have sent total (in the previous round of sending) the process starts over again starting from the "begin to send" point above (this is because we don't want to reset our minimum and maximum values). Variables such as the one representing total updates sent from lists, total updates sent from each particular list, and number of resent updates you have received from yourself should all be set back to 0    

This flow control works in that it allows processes to have a break to process messages instead of just sending all of their messages at one time. Additionally, it is important to note that this resending of udpdates is initially triggeed by a merge/partition occurring (thus the variables discussed in the second bullet point above are set during this time). Essentially, resending starts in the reconciliation process but persists through the rest of the program until all of the updates that needed to be sent have been sent.


#### Deleting Messages
We currently check if we can delete anything every 500 (new) updates that we receive (we choose this number because we decided that give we can have infinite clients and infinite emails per client we needed a fairly large number). We increment a counter (`num_updates_received`) every time we receive a new update. Once this number reaches our threshold for deleting updates, we loop through each column in our local merge matrix to find the minimum value in each column. We know that this is the message index that all processes have received up to from a particular process, and thus we are safe to delete everything up to and including that message index in the corresponding column in our `array_of_updates_list`. We do this by simply calling our `remove_from_beginning` function whle the message index on the update we are currently looking at is less than or equal to the message index that we want to delete up to and including. Also note that before we start deleting we must check to make sure that all values in our `done_sending_for_merge` array true, meaning that we have sent everything we needed to send for the last reconciliation (because it would be unfortunate to delete updates that you still want to send).    


## Discussion on Results
* Our mail server appears to work without error. They are resilient to network partitions and merges, and we have exhaustively tested it on many cases and combinations of merges/partitions/reads/deletions and believe them to work correctly. 
* We believe that we send as few messages as possible within our program. We do this by:
  * Having only a single server, the one that knows the most information, multicast necessary updates to other servers, and having that server send as few updates as possible (ie. starting at the update for the server who knows the least amount of information)
  * Implementing flow control when reconciling to prevent the Spread buffer from being exceeded and messages from being dropped
  * Bundling headers sent to send multiple headers on a single packet when sending headers to the client


## Additional Notes
This document was generated using markdown and converted to .pdf using a md-to-pdf converter tool.

