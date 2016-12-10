/*
Sarah Sukardi
Mariya Kazachkova
Distributed Systems: Final Project
December 9, 2016
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h> 
#include <netdb.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <assert.h>

#include "generic_linked_list.h"

#define PORT                   10160

// ******************* VARIABLES ******************* //
#define MAX_NAME_LEN           50
#define MAX_MESS_LEN           1000
#define MAX_PACKET_LEN         3000
#define MAX_HEADERS_IN_PACKET  10
#define NUM_SERVERS            5

#define NUM_FOR_DELETE_UPDATES 5

// **************** DATA STRUCTURES **************** //

// Lamport timestamp with additional server information
// * attached to emailinfos and updates
typedef struct timestamp {
  int         counter;
  int         machine_index;
  int         message_index;
} TimeStamp;


// Information associated with an email
// * attached to emails
typedef struct emailinfo {
  char        to_field[MAX_NAME_LEN];
  char        from_field[MAX_NAME_LEN];
  char        subject[MAX_NAME_LEN];
  char        message[MAX_MESS_LEN];  
  TimeStamp   timestamp;
} EmailInfo;


// Emails themselves
// * Attached to updates, as well as info_for_server messages
typedef struct email {
  EmailInfo   emailInfo;
  bool        read;
  bool        deleted;
  bool        exists;
} Email;


// The update struct
// * sent only between servers 
typedef struct update {
  int         type;                       // 10 is a new email, 
                                          // 11 is an email read, 
                                          // 12 is an email deleted,
                                          // 13 is new user created
  TimeStamp   timestamp;
  Email       email;                      // used for new emails
  char        user_name[MAX_NAME_LEN];    // used for new user created
  TimeStamp   timestamp_of_email;         // used for email read or email deleted

  int         index_of_machine_resending_update; //will need to inialize to -1 for all updates when we originally send them 
} Update;


// Mergematrix used for reconciliation during merges and repartitions (membership changes)
// * sent only between servers
typedef struct mergematrix {
  int type;                               // 20 is for the matrix
  int machine_index;                      // index from which it came
  int matrix[NUM_SERVERS][NUM_SERVERS];   // the 2-dimensional 5 x 5 reconciliation matrix
} MergeMatrix;


// User struct (a linked list of users held server-side)
// * held in server, not sent anywhere
typedef struct user {
  char  name[MAX_NAME_LEN];
  List  email_list;
} User;


// Information from clients
// * sent from clients to servers only
typedef struct info_for_server {
  int       type;                         // 1 is change of user
                                          // 2 is connect to server
                                          // 3 is list headers 
                                          // 4 is mail message to user
                                          // 5 is delete message
                                          // 6 is read message 
                                          // 7 is print membership
  char      user_name[MAX_NAME_LEN];      // for new user (?)
  Email     email;
  TimeStamp message_to_read;              //the timestamp of the EMAIL to read.
  TimeStamp message_to_delete;            //the timestamp of the EMAIL to delete.
} InfoForServer;


// Header struct
// * sent from in arrays on HeaderForClient objects
typedef struct header {
  int       message_number;                 
  char      sender[MAX_NAME_LEN];
  char      subject[MAX_NAME_LEN];
  bool      read;                         // whether the email was read or not
  TimeStamp timestamp;                    //the lamport timestamp of the email associated with the header
} Header;


// HeaderForClient struct
// * sent from servers to clients only
typedef struct header_for_client {
  int    type;                             // 1 is list header
  Header headers[MAX_HEADERS_IN_PACKET];   // used for sending headers
  bool   done;                             // might be used for telling the client whether it needs to receive another one of these 
} HeaderForClient;


// InfoForClient struct
// * sent from servers to clients only
typedef struct info_for_client {
  int    type;                                       
                                                     // 2 is specific email body to send
                                                     // 3 is print membership
                                                     // 4 is "client would like to join" message
  
  Email  email;                                      // used for sending specific email body
  char   memb_identities[NUM_SERVERS][MAX_NAME_LEN]; // used for sending membership identities
  char   client_server_group_name[MAX_NAME_LEN];     // used for sending the group name for "client would like to join" message
} InfoForClient;

