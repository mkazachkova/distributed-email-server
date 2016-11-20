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

#define PORT            10050

// ******************* VARIABLES ******************* //
#define MAX_NAME_LEN    50
#define MAX_MESS_LEN    1000

// **************** DATA STRUCTURES **************** //
typedef struct email {
  char[MAX_NAME_LEN] to_field;
  char[MAX_NAME_LEN] from_field;
  char[MAX_NAME_LEN] subject;
  char[MAX_MESS_LEN] message;
  bool read;
  bool deleted;
  TimeStamp timestamp;
} Email;

typedef struct update {
  int type; //1 is a new email, 2 is an email read, 3 is an email deleted 4 is new user created
  TimeStamp timestamp;
  //more stuff to come
  Email email;                    //used for new emails
  char[MAX_NAME_LEN] user_name;   //used for new user created
  TimeStamp timestamp_of_email;   //used for email read or email deleted
} Update;

typedef struct timestamp {
  int counter;
  int machine_index;
  int message_index;
} TimeStamp;

typedef struct user {
  char[MAX_NAME_LEN] name;

} User;


