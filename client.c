/*
Sarah Sukardi
Mariya Kazachkova
Distributed Systems: Final Project
December 9, 2016
*/

#include "sp.h"
#include "net_include.h"

#define int32u unsigned int

static char         Client_name[80];
static char         Spread_name[80];

static char         Private_group[MAX_GROUP_NAME];
static mailbox      Mbox;
static int          Num_sent;

static int          To_exit = 0;

#define MAX_MESSLEN 102400
#define MAX_VSSETS  10
#define MAX_MEMBERS 100

static void Print_menu();
static void User_command();
static void Read_message();
static void Usage( int argc, char *argv[] );
static void Print_help();
static void Bye();
static void print_header(Header *header);

//Variables that I (Sarah) added
static int  curr_server = -1;
static char curr_user[MAX_NAME_LEN];
static char hardcoded_server_names[NUM_SERVERS][MAX_NAME_LEN];


static bool logged_in = false;


int main(int argc, char *argv[]) {
  int     ret;
  int     mver, miver, pver;
  sp_time test_timeout;

  
  test_timeout.sec = 5;
  test_timeout.usec = 0;

  //Initialize hardcoded server names
  for (int i = 0; i < NUM_SERVERS; i++) {
    char temp_server_num[MAX_NAME_LEN];
    strcpy(hardcoded_server_names[i], "ssukard1mkazach1_server_");
    sprintf(temp_server_num, "%d", i + 10);
    strcat(hardcoded_server_names[i], temp_server_num);

    printf("%s\n", hardcoded_server_names[i]); // for debug
  }

  Usage(argc, argv);

  if (!SP_version(&mver, &miver, &pver)) {
    printf("main: Illegal variables passed to SP_version()\n");
    Bye();
  }
  
  printf("Spread library version is %d.%d.%d\n", mver, miver, pver);

  printf("Client name = %s\n", Client_name);
  ret = SP_connect_timeout(Spread_name, Client_name, 0, 1, &Mbox, Private_group, test_timeout);
  if(ret != ACCEPT_SESSION) {
    SP_error(ret);
    Bye();
  }

  printf("Client: connected to %s with private group %s\n", Spread_name, Private_group );

  E_init();
  E_attach_fd(0, READ_FD, User_command, 0, NULL, LOW_PRIORITY);
  E_attach_fd(Mbox, READ_FD, Read_message, 0, NULL, HIGH_PRIORITY);
  Print_menu();

  printf("\nUser> ");
  fflush(stdout);

  Num_sent = 0;
  E_handle_events();

  return(0);
}

static void Print_menu() {
  printf("\n");
  printf("==========\n");
  printf("User Menu:\n");
  printf("----------\n");
  printf("\n");
  printf("\tu <username> -- login as a user\n");
  printf("\tc <#>        -- connect to a specific mail server\n");
  printf("\tl            -- list headers of received mail\n");
  printf("\tm            -- mail a message to a user\n");
  printf("\td <#>        -- delete a message\n");
  printf("\tr <#>        -- read a message\n");
  printf("\tv            -- print membership identities\n");
  printf("\n");
  printf("\tq -- quit\n");
  fflush(stdout);
}

//Prints the menu
static void User_command() {
  char         command[130];
  char         mess[MAX_MESSLEN];
  char         to[MAX_NAME_LEN];
  char         subject[MAX_NAME_LEN];
  char         group[80];
  unsigned int mess_len;
  //char         groups[10][MAX_GROUP_NAME];
  //int          num_groups;

  int ret;
  int i;

  for(i = 0; i < sizeof(command); i++) {
    command[i] = 0;
  }

  if(fgets(command, 130, stdin) == NULL) {
    Bye();  
  }

  //Switch statement depending on type of message received
  switch(command[0]) {
  ///////////////////////////////////// LOGIN WITH A USERNAME ///////////////////////////////////
  case 'u': {
    ret = sscanf( &command[2], "%s", curr_user);
    printf("this is curr user: %s\n", curr_user);
    
    if (logged_in) {
      //alert the server that a (potentially) new user might need to be created
      InfoForServer *new_user_request = malloc(sizeof(InfoForServer));
      new_user_request->type = 2; //for new user
      sprintf(new_user_request->user_name, "%s",curr_user); //to be changed when we implement getting the user name
      
      SP_multicast(Mbox, AGREED_MESS, hardcoded_server_names[curr_server], 2, sizeof(InfoForServer), (char*)new_user_request);
      free(new_user_request);
    } else {
      // this is the client's first time connecting to a mail server
      logged_in = true;
    }
    
    break;
  }

  //////////////////////////////// CONNECT TO A SPECIFIC MAIL SERVER ////////////////////////////
  case 'c': {
    if (!logged_in) {
      printf("Must log in before connecting to server.\n");
      break;
    }
    ret = sscanf(&command[2], "%s", group);
    curr_server = atoi(group) - 1;
      
    if (ret < 1) {
      printf(" invalid group \n");
      break;
    }

    //this isn't doing anything
    // unsigned int message_length = strlen(hardcoded_server_names[curr_server]);
    // printf("Message of length %d with contents %s\n", message_length, hardcoded_server_names[curr_server]);
  
    InfoForServer *connect_server_request = malloc(sizeof(InfoForServer));
    connect_server_request->type = 2; //for new user
    sprintf(connect_server_request->user_name, "%s",curr_user); //to be changed when we implement getting the user name
      
    SP_multicast(Mbox, AGREED_MESS, hardcoded_server_names[curr_server], 2, sizeof(InfoForServer), (char*)connect_server_request);
    free(connect_server_request);

    break;
  }
      
  ////////////////////////////// LIST THE HEADERS OF RECEIVED MAIL ////////////////////////////
  case 'l': {
    InfoForServer *header_request = malloc(sizeof(InfoForServer));
    header_request->type = 3; //for a list headers of received mail request
    sprintf(header_request->user_name, "%s", curr_user);  //populate who is sending the email
    
    SP_multicast(Mbox, AGREED_MESS, hardcoded_server_names[curr_server], 2, sizeof(InfoForServer), (char*) header_request);
    free(header_request);
    
    break;
  }
      
  /////////////////////////////////// MAIL A MESSAGE TO A USER ////////////////////////////////
  case 'm': {
    //get TO: field from command line
    printf("To: ");

    fgets(&to[0], MAX_NAME_LEN, stdin);
    int to_len = strlen(to);
    to[to_len-1] = '\0';
      
    //get SUBJECT: field from command line
    printf("Subject: ");

    fgets(&subject[0], MAX_NAME_LEN, stdin);
    int subject_len = strlen(subject);
    subject[subject_len - 1] = '\0';
      
    //get MESSAGE: field from command line
    printf("Message: ");
    mess_len = 0;
    while (mess_len < MAX_MESS_LEN) {
      if (fgets(&mess[mess_len], 200, stdin) == NULL) {
        Bye();
      }

      //Message ends when user presses enter twice
      if (mess[mess_len] == '\n') {
        break;
      }
        
      mess_len += strlen(&mess[mess_len]);
    }

    //for debug
    printf("this is to: %s\nThis is subject: %s\nThis is message: %s\n", to, subject, mess);

    //Send the new message to the server
    InfoForServer *new_message_request = malloc(sizeof(InfoForServer));
    new_message_request->type = 4; //type for new email request
    sprintf(new_message_request->email.emailInfo.to_field,    "%s", to);
    sprintf(new_message_request->email.emailInfo.subject,     "%s", subject);
    sprintf(new_message_request->email.emailInfo.from_field,  "%s", curr_user);
    sprintf(new_message_request->email.emailInfo.message,     "%s", mess);

    //Send request to server
    SP_multicast(Mbox, AGREED_MESS, hardcoded_server_names[curr_server], 2, sizeof(InfoForServer), (char*)new_message_request);
    free(new_message_request);

    break;   
  }
    
  ////////////////////////////////// DELETE A MAIL MESSAGE ////////////////////////////////
  case 'd': {
    //TODO: This method has not been implemented or tested yet
    
    InfoForServer *delete_request = malloc(sizeof(InfoForServer));
    delete_request->type = 5;                             //for a print membership request
    sprintf(delete_request->user_name, "%s", curr_user);  //populate the user_name field for who is sending the email

    //Retrieve which email we want to delete from command-line input
    sscanf(&command[2], "%s", group);
    int to_be_deleted = atoi(group);

    //set the request's message_to_read field
    delete_request->message_to_delete = to_be_deleted; 
    SP_multicast(Mbox, AGREED_MESS, hardcoded_server_names[curr_server], 2, sizeof(InfoForServer), (char*) delete_request);

    free(delete_request);

    break;

  }
    
  ////////////////////////////////////// READ AN EMAIL /////////////////////////////////////
  case 'r': {
    InfoForServer *read_request = malloc(sizeof(InfoForServer));
    read_request->type = 6;                             //for a print membership request
    sprintf(read_request->user_name, "%s", curr_user);  //populate the user_name field for who is sending the email

    //Retrieve which email we want to read from command-line input
    sscanf(&command[2], "%s", group);
    int to_be_read = atoi(group);

    //set the request's message_to_read field
    read_request->message_to_read = to_be_read; 
    SP_multicast(Mbox, AGREED_MESS, hardcoded_server_names[curr_server], 2, sizeof(InfoForServer), (char*) read_request);

    free(read_request);
    
    break;
  }
      
  /////////////////////////// PRINT MEMBERSHIP OF THE MAIL SERVERS /////////////////////////
  ////////////////////// IN THE CURRENT MAIL SERVER'S NETWORK COMPONENT ////////////////////
  case 'v': {
    InfoForServer *membership_request = malloc(sizeof(InfoForServer));
    membership_request->type = 7; //for a print membership request
    sprintf(membership_request->user_name, "%s", curr_user);  //populate who is sending the email
    
    SP_multicast(Mbox, AGREED_MESS, hardcoded_server_names[curr_server], 2, sizeof(InfoForServer), (char*) membership_request);
    
    free(membership_request);
    break;
  }
    
  ///////////////////////////////////// EXIT THE PROGRAM ///////////////////////////////////
  case 'q': {
    Bye();
    break;
  }
      
  //////////////////////////////////////// DEFAULT CASE ////////////////////////////////////
  default: {
    printf("\nYou input an undefined commnand. Printing menu...\n");
    Print_menu();
    break;
  }
  }
  
  printf("\nUser> ");
  fflush(stdout);
}


//This method is triggered whenever a client RECEIVES a message from a server
static void Read_message() {

  static  char              sender[MAX_GROUP_NAME];
          //char              mess[MAX_MESSLEN];
          char              target_groups[MAX_MEMBERS][MAX_GROUP_NAME];
          //membership_info   memb_info;
          //vs_set_info       vssets[MAX_VSSETS];
          //unsigned int      my_vsset_index;
          //int               num_vs_sets;
          //char              members[MAX_MEMBERS][MAX_GROUP_NAME];
          int               num_groups;
          int               service_type;
          int16             mess_type;
          int               endian_mismatch;
          //int               i,j;
          //int               ret;

          service_type = 0;


  //should be triggered when we receive something
  InfoForClient *info = malloc(sizeof(InfoForClient)); 
  SP_receive( Mbox, &service_type, sender, 100, &num_groups, target_groups,
              &mess_type, &endian_mismatch, sizeof(InfoForClient), (char*)info); 

  
  printf("We have received a new info for client object of type %d\n", info->type);


  ////////////////////////// LIST HEADERS MESSAGE RECEIVED //////////////////////////
  //NOTE: Make sure to make all EMPTY HEADERS have a message_number of -1

  if (info->type == 1) { //this is to "list headers" message received from server 
    //TODO: implement!

    for (int i = 0; i < MAX_HEADERS_IN_PACKET; i++) {
      if (info->headers[i].message_number != -1) {
        print_header(&(info->headers[i]));
        printf("\n");
      }
    }

  //////////////////////// PRINT EMAIL BODY MESSAGE RECEIVED ////////////////////////
  } else if (info->type == 2) { //"print email body" message received from server (mark as read server side)
    printf("To: %s\nFrom: %s\nSubject: %s\nBody: %s\n",
            info->email.emailInfo.to_field, info->email.emailInfo.from_field, 
            info->email.emailInfo.subject, info->email.emailInfo.message);


  //////////////////////// PRINT MEMBERSHIP MESSAGE RECEIVED ////////////////////////
  } else if (info->type == 3) { //print memberships

    //NOTE: Make sure to set unused servers to the empty string in server.c
    for (int i = 0; i < NUM_SERVERS; i++) {
      //if not the empty string, print the server
      if (strcmp(info->memb_identities[i], "") != 0) {
        printf("%s\n", info->memb_identities[i]);
      }
    }    
  }

  

  /*  
  ret = SP_receive( Mbox, &service_type, sender, 100, &num_groups, target_groups, 
    &mess_type, &endian_mismatch, sizeof(mess), mess );
  printf("\n============================\n");
  if (ret < 0) {
    if ( (ret == GROUPS_TOO_SHORT) || (ret == BUFFER_TOO_SHORT) ) {
      service_type = DROP_RECV;
      printf("\n========Buffers or Groups too Short=======\n");
      ret = SP_receive( Mbox, &service_type, sender, MAX_MEMBERS, &num_groups, target_groups, 
                        &mess_type, &endian_mismatch, sizeof(mess), mess );
    }
  }
  if (ret < 0) {
    if (!To_exit) {
      SP_error(ret);
      printf("\n============================\n");
      printf("\nBye.\n");
    }
    exit(0);
  }

  //Regular type of message received (non-membership)
  if (Is_regular_mess(service_type)) {
    mess[ret] = 0;
    
    //for debugging 
    if      (Is_unreliable_mess(service_type)) printf("received UNRELIABLE\n");
    else if (Is_reliable_mess(  service_type)) printf("received RELIABLE\n");
    else if (Is_fifo_mess(      service_type)) printf("received FIFO\n");
    else if (Is_causal_mess(    service_type)) printf("received CAUSAL\n");
    else if (Is_agreed_mess(    service_type)) printf("received AGREED\n");
    else if (Is_safe_mess(      service_type)) printf("received SAFE\n");

    printf("message from %s, of type %d, (endian %d) to %d groups \n(%d bytes): %s\n",
      sender, mess_type, endian_mismatch, num_groups, ret, mess );

    //TODO: process messages received




  //Change of membership detected
  } else if (Is_membership_mess(service_type)) {
    ret = SP_get_memb_info( mess, service_type, &memb_info );
    if (ret < 0) {
      printf("BUG: membership message does not have valid body\n");
      SP_error( ret );
      exit( 1 );
    }

    if (Is_reg_memb_mess(service_type)) {
      printf("Received REGULAR membership for group %s with %d members, where I am member %d:\n",
        sender, num_groups, mess_type );

      for (i = 0; i < num_groups; i++) {
        printf("\t%s\n", &target_groups[i][0]);
      }

      printf("grp id is %d %d %d\n",memb_info.gid.id[0], memb_info.gid.id[1], memb_info.gid.id[2] );

      if (Is_caused_join_mess(service_type)) {
        printf("Due to the JOIN of %s\n", memb_info.changed_member);
      } else if (Is_caused_leave_mess( service_type)) {
        printf("Due to the LEAVE of %s\n", memb_info.changed_member);
      } else if (Is_caused_disconnect_mess( service_type)) {
        printf("Due to the DISCONNECT of %s\n", memb_info.changed_member);
      } else if (Is_caused_network_mess( service_type)) {
        printf("Due to NETWORK change with %u VS sets\n", memb_info.num_vs_sets);
        num_vs_sets = SP_get_vs_sets_info( mess, &vssets[0], MAX_VSSETS, &my_vsset_index);
        if (num_vs_sets < 0) {
          printf("BUG: membership message has more then %d vs sets. Recompile with larger MAX_VSSETS\n", MAX_VSSETS);
          SP_error(num_vs_sets);
          exit(1);
        }


        for (i = 0; i < num_vs_sets; i++) {
          printf("%s VS set %d has %u members:\n",
                 (i == my_vsset_index) ? "LOCAL" : "OTHER", i, vssets[i].num_members);
          ret = SP_get_vs_set_members(mess, &vssets[i], members, MAX_MEMBERS);
          if (ret < 0) {
            printf("VS Set has more then %d members. Recompile with larger MAX_MEMBERS\n", MAX_MEMBERS);
            SP_error(ret);
            exit(1);
          }
          for (j = 0; j < vssets[i].num_members; j++) {
            printf("\t%s\n", members[j] );
          }
        }
      }

    } else if (Is_transition_mess(service_type)) {
      printf("received TRANSITIONAL membership for group %s\n", sender);
    } else if (Is_caused_leave_mess(service_type)) {
      printf("received membership message that left group %s\n", sender);
    } else {
      printf("received incorrect membership message of type 0x%x\n", service_type);
    }
  } else if (Is_reject_mess(service_type)) {
    printf("REJECTED message from %s, of servicetype 0x%x messtype %d, (endian %d) to %d groups \n(%d bytes): %s\n",
      sender, service_type, mess_type, endian_mismatch, num_groups, ret, mess );
  } else {
    printf("received message of unknown message type 0x%x with ret %d\n", service_type, ret);
  }

  printf("\n");
  printf("User> ");*/
  fflush(stdout); 
}

static void print_header(Header *header) {
  printf("%d. From: %s\nSubject: %s\n", header->message_number, header->sender, header->subject);
}

//Takes in command-line args for spread name and user
static void Usage(int argc, char *argv[]) {
  sprintf(Client_name, "client_user_mk_ss");
  sprintf(Spread_name, "10050");

  while (--argc > 0) {
    argv++;

    if(!strncmp(*argv, "-u", 2)) { //if the next 2 characters are equal to -u (is what it means)
      if (argc < 2) { Print_help(); } //apparently invalid amt. of args
      strcpy(Client_name, argv[1]);
      argc--; argv++;
    } else if (!strncmp(*argv, "-r", 2)) {
      strcpy(Client_name, "");
    } else if (!strncmp(*argv, "-s", 2)) {
      if (argc < 2) { Print_help(); }
      strcpy(Spread_name, argv[1]); 
      argc--; argv++;
    } else {
      Print_help();
    }
  }
}

//Print the help menu
static void Print_help() {
  printf( "Usage: spuser\n%s\n%s\n%s\n",
    "\t[-u <user name>]  : unique (in this machine) user name",
    "\t[-s <address>]    : either port or port@machine",
    "\t[-r ]             : use random user name");
  exit( 0 );
}

//Exit the program
static void Bye() {
  To_exit = 1;
  printf("\nBye.\n");
  SP_disconnect( Mbox );
  exit( 0 );
}
