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

#define PORT            10050

// ******************* VARIABLES ******************* //
#define MAX_NAME_LEN    50
#define MAX_MESS_LEN    1000
#define MAX_PACKET_LEN  2000
#define NUM_SERVERS     5

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
  int         updates_array[NUM_SERVERS]; // one-dimensional array containing server's current "view" of updates received
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
  int type;                               // 2 is new user
                                          // 3 is list headers 
                                          // 4 is mail message to user
                                          // 5 is delete message
                                          // 6 is read message 
                                          // 7 is print membership
  char user_name[MAX_NAME_LEN];           // for new user (?)
  Email email;
  int message_to_delete;
  int message_to_read;
} InfoForServer;
