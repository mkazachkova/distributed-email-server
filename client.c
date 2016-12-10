/*
Sarah Sukardi
Mariya Kazachkova
Distributed Systems: Final Project
December 9, 2016
*/

#include "sp.h"
#include "net_include.h"

#define int32u unsigned int

#define MAX_MESSLEN 102400
#define MAX_VSSETS  10
#define MAX_MEMBERS 100

//Their variables (from class_user.c)
static char         Client_name[80];
static char         Spread_name[80];

static char         Private_group[MAX_GROUP_NAME];
static mailbox      Mbox;
static int          Num_sent;

static int          To_exit = 0;


//Methods
static void       Print_menu();
static void       User_command();
static void       Read_message();
static void       Usage(int argc, char *argv[]);
static void       Print_help();
static void       Bye();
static void       print_header(Header *header);
static void       add_header(Header *header);
static TimeStamp* get_timestamp_associated_with_header(int header_num_requested);
static int        compare_header_num(void* h1, void* h2);

//Variables that I (Sarah) added
static int  curr_server = -1;
static char curr_user[MAX_NAME_LEN];
static char hardcoded_server_names[NUM_SERVERS][MAX_NAME_LEN];
static char curr_group_connected_to[MAX_NAME_LEN] = "";

static bool logged_in = false;
static bool connected_to_server = false;
static bool can_print_user = true;

//Persistent storage variables
List        headers_list;


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
  }

  //Initialize header list to be empty linked list
  create_list(&headers_list, sizeof(Header));

  Usage(argc, argv);

  if (!SP_version(&mver, &miver, &pver)) {
    printf("main: Illegal variables passed to SP_version()\n");
    Bye();
  }
  
  printf("Spread library version is %d.%d.%d\n", mver, miver, pver);

  printf("Client name = %s\n", Client_name);
  ret = SP_connect_timeout(Spread_name, NULL, 0, 1, &Mbox, Private_group, test_timeout);
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

    empty_list(&headers_list);
    
    /*DOUBLE CHECK THIS. MY THINKING IS WE SHOULDNT SEND ANYTHING TO SERVER IF NOT CONNECTED*/
    if (logged_in && connected_to_server) {       
      //alert the server that a (potentially) new user might need to be created
      InfoForServer *new_user_request = malloc(sizeof(InfoForServer));
      new_user_request->type = 1; //for new user
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
      printf("You must log in before connecting to the server.\n");
      break;
    }

    //Parse input
    ret = sscanf(&command[2], "%s", group);
    curr_server = atoi(group) - 1;
      
    if (ret < 1) {
      printf("Invalid group. \n");
      break;
    }

    // Set curr_group_connected_to back to the empty string
    if (strcmp(curr_group_connected_to, "") != 0) { // some group was previously connected to
      SP_leave(Mbox, curr_group_connected_to);
      strcpy(curr_group_connected_to, ""); //curr_group_connected_to is blank again!
      connected_to_server = false; //since breaking connection here
    }
    
    //Populate InfoForServer struct with information
    InfoForServer *connect_server_request = malloc(sizeof(InfoForServer));
    connect_server_request->type = 2; //for new user
    sprintf(connect_server_request->user_name, "%s",curr_user); //to be changed when we implement getting the user name

    //Send initial request to log in to the server
    int ret = SP_multicast(Mbox, AGREED_MESS, hardcoded_server_names[curr_server], 2, sizeof(InfoForServer), (char*)connect_server_request);

    if (ret < 0) {
      SP_error(ret);
      printf("Error occurred in SP_multicast.\n");
      exit(1);
    }

    // Wait for secs_to_wait seconds (max) to receive a response from the server!
    // Start timer 
    int start_time = time(NULL);
    int secs_to_wait = 2;
    bool connection_established = false;

    //Loop for 1 seconds at 1/10-second intervals, checking if the mailbox has stuff in it
    while (time(NULL) < (start_time + secs_to_wait)) {
      //Check if the mailbox has anything in it
      ret = SP_poll(Mbox);

      //Error when ret < 0  
      if (ret < 0) {  
        printf("Error occurred during SP_poll.\n");
        exit(1);

      //There is something in the mailbox to receive!    
      } else if (ret > 0) { 
        InfoForClient *info = malloc(sizeof(InfoForClient));

        int               service_type;
        char              sender[MAX_GROUP_NAME];
        int               num_groups;
        char              target_groups[MAX_MEMBERS][MAX_GROUP_NAME];
        int16             mess_type;
        int               endian_mismatch;
        
        //Receive information and check if it's the correct type!
        SP_receive(Mbox, &service_type, sender, 100, &num_groups, target_groups,
                   &mess_type, &endian_mismatch, sizeof(InfoForClient), (char*)info);
        

        //Ignore the leave message from the server it was once connected to
        ////!!! SUPER IMPORTANT !!!!
        if (Is_caused_leave_mess(service_type)) {
          free(info);
          continue;
        }
        
        if (info->type == 4) {
          //Now we connect to whatever name the server has sent back to us
          int join = SP_join(Mbox, info->client_server_group_name);
          strcpy(curr_group_connected_to, info->client_server_group_name);

          connected_to_server = true;
          printf("Successfully connected to server!\n");
          assert(join == 0);
          free(info);
        } else {
          printf("You received SOMETHING but it's incorrect. Exiting... \n");
          exit(1);
        }

        connection_established = true;
        break; //exit out of loop

      //There is nothing in the mailbox to receive, loop again
      } else {
        usleep(100000); // 1/10 of a second
      }

    }

    if (!connection_established) {
      printf("The server is currently unavailable. Please try another server.\n");      
    }
    
    free(connect_server_request);

    break;
  }
      
  ////////////////////////////// LIST THE HEADERS OF RECEIVED MAIL ////////////////////////////
  case 'l': {
    if (!connected_to_server) {
      printf("You must connect to a server before listing your inbox.\n");
      break;
    }

    InfoForServer *header_request = malloc(sizeof(InfoForServer));
    header_request->type = 3; //for a list headers of received mail request
    sprintf(header_request->user_name, "%s", curr_user);  //populate who is sending the email
    
    SP_multicast(Mbox, AGREED_MESS, hardcoded_server_names[curr_server], 2, sizeof(InfoForServer), (char*) header_request);
    free(header_request); 

    //reset the persistent headers_list back to an empty list
    empty_list(&headers_list);

    printf("User Name: %s\n", curr_user);
    printf("Responding Server: %d\n", curr_server + 1);
    break;
  }
      
  /////////////////////////////////// MAIL A MESSAGE TO A USER ////////////////////////////////
  case 'm': {
    if (!connected_to_server) {
      printf("You must connect to a server before sending an email.\n");
      break;
    }
    
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
    if (!connected_to_server) {
      printf("You must connect to a server before deleting message.\n");
      break;
    }
    
    if (headers_list.num_nodes == 0) {
      printf("Email deletion index invalid. NOTE: You must first list headers in order to choose to delete a particular message.\n");
      break;
    }

    //Retrieve which email we want to delete from command-line input
    sscanf(&command[2], "%s", group);
    int to_be_deleted = atoi(group);

    //Make sure the user inputs a valid email number to delete
    if (to_be_deleted > headers_list.num_nodes || to_be_deleted <= 0) {
      printf("You input an invalid email number to delete.\n");
      break;
    }

    InfoForServer *delete_request = malloc(sizeof(InfoForServer));
    delete_request->type = 5;                             //for a print membership request
    sprintf(delete_request->user_name, "%s", curr_user);  //populate the user_name field for who is sending the email

    //set the request's message_to_delete field
    TimeStamp *nth_timestamp = get_timestamp_associated_with_header(to_be_deleted);
    assert(nth_timestamp != NULL); //this should never be null since we error check it's in bounds above

    delete_request->message_to_delete = *nth_timestamp; 
    SP_multicast(Mbox, AGREED_MESS, hardcoded_server_names[curr_server], 2, sizeof(InfoForServer), (char*) delete_request);

    free(delete_request);

    break;

  }
    
  ////////////////////////////////////// READ AN EMAIL /////////////////////////////////////
  case 'r': {
    if (!connected_to_server) {
      printf("You must connect to a server before reading an email.\n");
      break;
    }
    
    if (headers_list.num_nodes == 0) {
      printf("Email read index invalid. NOTE: You must first list headers in order to choose to read a particular message.\n");
      break;
    }

    //Retrieve which email we want to read from command-line input
    sscanf(&command[2], "%s", group);
    int to_be_read = atoi(group);

    //Make sure the user inputs a valid email number to read
    if (to_be_read > headers_list.num_nodes || to_be_read <= 0) {
      printf("You input an invalid email number to read.\n");
      break;
    }

    InfoForServer *read_request = malloc(sizeof(InfoForServer));
    read_request->type = 6;                             //for a print membership request
    sprintf(read_request->user_name, "%s", curr_user);  //populate the user_name field for who is sending the email

    //set the request's message_to_read field
    TimeStamp *nth_timestamp = get_timestamp_associated_with_header(to_be_read);
    assert(nth_timestamp != NULL); //this should never be null since we error check it's in bounds above

    read_request->message_to_read = *nth_timestamp;
    SP_multicast(Mbox, AGREED_MESS, hardcoded_server_names[curr_server], 2, sizeof(InfoForServer), (char*) read_request);

    free(read_request);
    
    break;
  }
      
  /////////////////////////// PRINT MEMBERSHIP OF THE MAIL SERVERS /////////////////////////
  ////////////////////// IN THE CURRENT MAIL SERVER'S NETWORK COMPONENT ////////////////////
  case 'v': {
    if (!connected_to_server) {
      printf("You must connect to a server before listing members.\n");
      break;
    }
    
    InfoForServer *membership_request = malloc(sizeof(InfoForServer));
    membership_request->type = 7; //for a print membership request
    sprintf(membership_request->user_name, "%s", curr_user);  //populate who is sending the email
    
    SP_multicast(Mbox, AGREED_MESS, hardcoded_server_names[curr_server], 2, sizeof(InfoForServer), (char*) membership_request);
    
    free(membership_request);
    break;
  }
    
  ///////////////////////////////////// EXIT THE PROGRAM ///////////////////////////////////
  case 'q': {    
    //TODO: send message to server saying that you're disconnecting; don't wait for a response
    //server will receive this message
    //OR
    //maybe the server will automatically get a message? We need to test this.
    //low priority though
    Bye();
    break;
  }

  case 's': {
    printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
    break;
  }
      
  //////////////////////////////////////// DEFAULT CASE ////////////////////////////////////
  default: {
    printf("\nYou input an undefined command. Printing menu...\n");
    Print_menu();
    break;
  }
  }

  if (can_print_user) {
    printf("\nUser> ");
    fflush(stdout);
  }
}


//This method is triggered whenever a client RECEIVES a message from a server
static void Read_message() {

  static  char              sender[MAX_GROUP_NAME];
          char              target_groups[MAX_MEMBERS][MAX_GROUP_NAME];
          int               num_groups;
          int               service_type;
          int16             mess_type;
          int               endian_mismatch;

          service_type = 0;

  char *tmp_buf = malloc(MAX_PACKET_LEN);
  int ret = SP_receive(Mbox, &service_type, sender, 100, &num_groups, target_groups,
                   &mess_type, &endian_mismatch, MAX_PACKET_LEN, (char*)tmp_buf);

  //For debug
  // printf("\nATTN: MESSAGE RECEIVED W/ SERVICE TYPE: %d FROM %s SIZE %d\n", service_type, sender, ret);

  if (Is_transition_mess(service_type)) {
    free(tmp_buf);
    printf("User> ");
    fflush(stdout);
    return;
  }

  if (Is_caused_network_mess(service_type)) {
    printf("**********You have been disconnected from your server. Please try connecting to another one**********.\n");

    //Make curr_group_connected_to back to empty string
    SP_leave(Mbox, curr_group_connected_to);
    strcpy(curr_group_connected_to, "");

    //Set to false since we have been disconnected 
    connected_to_server = false; 
    
    free(tmp_buf);
    printf("User> ");
    fflush(stdout);
    return;
  }
  
  if (Is_caused_join_mess(service_type)) {
    free(tmp_buf);
    printf("User> ");
    fflush(stdout);
    return;
  }

  if (Is_caused_leave_mess(service_type)) {
    free(tmp_buf);
    printf("User> ");
    fflush(stdout);
    return;
  }

  
  int *type = (int*) tmp_buf;

  ////////////////////////// LIST HEADERS MESSAGE RECEIVED //////////////////////////
  //NOTE: Make sure to make all EMPTY HEADERS have a message_number of -1

  if (*type == 1) { //this is to "list headers" message received from server     
    //should be triggered when we receive something  
    HeaderForClient *info_header = (HeaderForClient *) tmp_buf;  

    bool done = info_header->done;
    
    for (int i = 0; i < MAX_HEADERS_IN_PACKET; i++) {
      if (info_header->headers[i].message_number != -1) {
        //print the header on the screen for the client
        print_header(&(info_header->headers[i])); 

        //add header to persistent headers linked list
        add_header(&(info_header->headers[i])); 
        printf("\n");
      }
    }
    
    //Receive the rest of the headers
    while (!done) {
      HeaderForClient *info_header_t = malloc(sizeof(HeaderForClient)); 
      SP_receive(Mbox, &service_type, sender, 100, &num_groups, target_groups,
                 &mess_type, &endian_mismatch, sizeof(HeaderForClient), (char*)info_header_t);

      for (int i = 0; i < MAX_HEADERS_IN_PACKET; i++) {
        if (info_header_t->headers[i].message_number != -1) {
          //print the header on the screen for the client
          print_header(&(info_header_t->headers[i]));

          //add header to persistent headers linked list
          add_header(&(info_header_t->headers[i])); 
          printf("\n");
        }
      }

      done = info_header_t->done;
      free(info_header_t);
      info_header_t = NULL;
    }

  //////////////////////// PRINT EMAIL BODY MESSAGE RECEIVED ////////////////////////
  } else if (*type == 2) { //"print email body" message received from server (mark as read server side)
    //should be triggered when we receive something
    InfoForClient *info = (InfoForClient *) tmp_buf;  

    if (info->email.deleted) {
      printf("You cannot read this message: It has already been deleted!\n");
    } else {      
      printf("****************************************\nTO: %s\nFROM: %s\nSUBJECT: %s\n\nBODY: %s****************************************\n",
             info->email.emailInfo.to_field, info->email.emailInfo.from_field, 
             info->email.emailInfo.subject, info->email.emailInfo.message);
    }

  //////////////////////// PRINT MEMBERSHIP MESSAGE RECEIVED ////////////////////////
  } else if (*type == 3) { //print memberships
    //should be triggered when we receive something
    InfoForClient *info = (InfoForClient *) tmp_buf;  

    //NOTE: Make sure to set unused servers to the empty string in server.c
    for (int i = 0; i < NUM_SERVERS; i++) {
      //if not the empty string, print the server
      if (strcmp(info->memb_identities[i], "") != 0) {
        printf("%s\n", info->memb_identities[i]);
      }
    }    
  } else {
    printf("SOMETHING UNDEFINED RECEIVED!!\n");
  }

  printf("\n");
  printf("User> ");
  fflush(stdout); 
}

static void print_header(Header *header) {
  printf("***************** MESSAGE NUMBER: %d *******************\n", header->message_number);
  printf("From: %s\nSubject: %s\nRead: %d\n", header->sender, header->subject, header->read);
  printf("--------------------------------------------------------\n");
}


//All headers should be added in consecutive order
static void add_header(Header *header) {
  add_to_end(&headers_list, (void*) header);
}


static TimeStamp* get_timestamp_associated_with_header(int num) {
  Header* found_header = (Header *) find(&headers_list, &num, compare_header_num);
  return &(found_header->timestamp);
} 


//Returns if two users have the same name
static int compare_header_num(void* h1, void* h2) {
  Header *header_in_linked_list = (Header*) h1;
  int *desired_header_num = (int*) h2;

  return (header_in_linked_list->message_number == *desired_header_num) ? 0 : 1;
}


//Takes in command-line args for spread name and user
static void Usage(int argc, char *argv[]) {
  sprintf(Client_name, "client_usero_mk_ss");
  sprintf(Spread_name, "10160");

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
