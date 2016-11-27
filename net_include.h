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

#include "generic_linked_list.h"

#define PORT            10050

// ******************* VARIABLES ******************* //
#define MAX_NAME_LEN    50
#define MAX_MESS_LEN    1000
#define NUM_SERVERS     5

// **************** DATA STRUCTURES **************** //
typedef struct email {
  EmailInfo           emailInfo;
  bool                read;
  bool                deleted;
} Email;

typedef struct emailinfo {
  char[MAX_NAME_LEN]  to_field;
  char[MAX_NAME_LEN]  from_field;
  char[MAX_NAME_LEN]  subject;
  char[MAX_MESS_LEN]  message;  
  TimeStamp           timestamp;
} EmailInfo;

typedef struct update {
  int                 type;               // 1 is a new email, 
                                          // 2 is an email read, 
                                          // 3 is an email deleted,
                                          // 4 is new user created
  TimeStamp           timestamp;
  Email               email;              //used for new emails
  char[MAX_NAME_LEN]  user_name;          //used for new user created
  TimeStamp           timestamp_of_email; //used for email read or email deleted
  int[NUM_SERVERS]    updates_array;
} Update;

typedef struct mergematrix {
  int machine_index;                      //index from which it came
  int[NUM_SERVERS][NUM_SERVERS] matrix;   //the 2-dimensional 5 x 5 reconciliation matrix
} MergeMatrix;

typedef struct timestamp {
  int                 counter;
  int                 machine_index;
  int                 message_index;
} TimeStamp;

typedef struct user {
  char[MAX_NAME_LEN]  name;
  List                *email_list;
} User;


