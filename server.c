/*
Sarah Sukardi
Mariya Kazachkova
Distributed Systems: Final Project
December 9, 2016
*/

#include "sp.h"
#include "net_include.h"

/**********RECEIVING************/
//CASE: received type 4 update (new user has been created)
//take name off of the update
//iterate through your user array and see if that user exists
//if not, create a new user object with the username from the update
//append update to the corresponding index to the machine that sent it (append to linked list)
//(do something with updating the counter)


//CASE: received type 1 update (new email)
//first, check if that email already exists. If it does, just store the update
//if email doesn't exist, take to field off of email and find that user in your user linked list
//if user doesn't exist, create new user using the to field (****double check this, might just display an error)
//if user to does exist, find that object is user linked list and access the email linked list of that user
//using the timestamps insert the email into the correct location for that email
//append update to the corresponding index to the machine that sent it (append to linked list)
//(do something with counter maybe)


//CASE: receive type 2 update (read)
//first, check if user whose mailbox you'll be modifiying actually exists or not
//if user doesn't exist, create new user object with name of user. Then, create a new node in the linked list of emails for that user
//with null data. Set the read bool equal to true for that specific email
//If user does exist, loop through linked list of email to find the email with that specific time stamp
//if email doesn't exist then create a new node and insert it into the linked list where the email should have been.
//set the data for this node to null, and set the bool for read to true.
//If email does exist, set the the read bool to true. If already true, then can set it to true again without anything going wrong


//CASE: receive type 3 update (delete)
//first, check if user whose mailbox you'll be modifiying actually exists or not
//if user doesn't exist, create new user object with name of user. Then, create a new node in the linked list of emails for that user
//with null data. Set the delete bool equal to true for that specific email
//If user does exist, loop through linked list of email to find the email with that specific time stamp
//if email doesn't exist then create a new node and insert it into the linked list where the email should have been.
//set the data for this node to null, and set the bool for delete to true.
//If email does exist, set the the delete bool to true. If already true, then can set it to true again without anything going wrong



/**********CLIENT FUNCTIONS**********/

//CASE: client login to server (new client)
//Client and server are put into private group (membership and spread stuff)
//Server x (server that user is on) creates a new user object and appends it to user linked list
//Server x sends all other servers (multicast) in its partition an update saying a new user has joined


//CASE: client login to server (returning client)
//Client and server in private group
//Server x iterates through user linked list and finds user object. No update is sent and no new user object is created


//CASE: client switches servers
//private group between cliet and original server is dropped
//private group between client and new server is created


//CASE: client wants to display all headers
//find specific use in linked list of users
//iterate through email linked list within the user and display (backwards) all emails with non-null data


//CASE: client sends an email
//client is prompted for to, subject, and message.
//Server x (the server the user is currently on) multicasts an update with type 1 signifiying a new email (first create email
//and update object; attach timestamp as needed)

//CASE: client wants to delete a message
//client selects email to delete after displaying the headers (deleting yth email)
//iterate from tail to find yth email; take time stamp off of this email and store as t. Change data in email to null
//note for above: need to iterate over non-null data
//server x (the server the user is currently on) sends an update (type 3) with a timestamp_of_email (this will be t) and its
//own timestamp as well 


//CASE: client reads an email
//client selects an email to read. The message of the email is diplayed to the client (iterate through linked list from tail
//and that the xth email ignoring null data). Server x sends an update (type 2) with the timestamp of the email that was read
//along with its own timestamp and multicasts it out

//CASE: client wants to print memberships
//use spread to print all servers currently in the partition


/**********WHEN MEMBERSHIP CHANGES**********/
//use anti entropy. boom. done

//we attach a 1d array with what we (as a server) know from every other server to every update that we send. When a server receives this
//they add it to their 2d array of what all servers know from all other servers.

//When there is a merge each server involved in the merge multicasts out a struct (merge_matrix) containing its own 2d matrix.

//When you receive one of these merge_matrix structs you iterate through it and compare each cell to the corresponding cell
//in your own 2d array. You take the max of the values and stre that as the cell in your 2d array. You must wait until you receive however many
//servers there are in the new partition worth of these merge_matrix structs. Ignore the merge_matrix struct you receive from yourself.

//once your 2d array is loaded with all of the new information, you must only consider the servers that you are in a partition with
//currently as possible sources of information (this is the vertical side of the array). You look at each column and find the max value (only
//considering the processes that are in the partition). The process with the max value is responsible for sending the updates to all other processes
//in the partition






#define int32u unsigned int

static char         Server[80];
static char         Spread_name[80];

static char         Private_group[MAX_GROUP_NAME];
static mailbox      Mbox;
static int          Num_sent;
static unsigned int Previous_len;

static char     group[80] = "ssukard1mkazach1_2"; 


char server_own_group[80] = "ssukard1mkazach1_server_";

int             my_machine_index;
int             num_machines;

static int          To_exit = 0;

int     ret;
int     mver, miver, pver;
sp_time test_timeout;

List users_list;
int merge_matrix[NUM_SERVERS][NUM_SERVERS];
int update_count = 0;
bool servers_in_partition[NUM_SERVERS] = { false };
int num_servers_in_partition = 0; //THIS MAY NOT BE CORRECT SO DO NOT USE
//#define MAX_MESSLEN 102400
//#define MAX_VSSETS  10
//#define MAX_MEMBERS 100


static void Respond_To_Message();
int compare_users(void* user1, void* user2);
bool create_user_if_nonexistent(char *name);
void print_user(void *user);
void print_email(void *user);




int main(int argc, char *argv[]) {


  //Create a linked list of users
  create_list(&users_list, sizeof(User));

  for (int i = 0; i < NUM_SERVERS; i++) {
    for (int j = 0; j < NUM_SERVERS; j++) {
      merge_matrix[i][j] = 0;
    }
  }
  
  my_machine_index =  atoi(argv[1]) - 1; //subtract 1 for correct indexing
  num_machines =      atoi(argv[2]);

  char my_machine_index_str[20];
  
  sprintf(my_machine_index_str, "%d", my_machine_index + 10);
  

  strcat(server_own_group, my_machine_index_str);
  
  printf("%s\n", server_own_group);

  
  sprintf(Server, "user_mk_ss");
  sprintf(Spread_name, "10050");

  if (!SP_version(&mver, &miver, &pver)) {
    printf("main: Illegal variables passed to SP_version()\n");
    //Bye();
  }

  printf("Spread library version is %d.%d.%d\n", mver, miver, pver);


  ret = SP_connect_timeout(Spread_name, Server, 0, 1, &Mbox, Private_group, test_timeout);
  if (ret != ACCEPT_SESSION) {
    SP_error(ret);
    //Bye();
  }

  printf("Server: connected to %s with private group %s\n", Spread_name, Private_group);

  ret = SP_join(Mbox, group);
  if (ret < 0) {
    SP_error(ret);
  }



  ret = SP_join(Mbox, server_own_group);
  if (ret < 0) {
    SP_error(ret);
  }

  printf("connected\n");

  E_init();
  E_attach_fd( Mbox, READ_FD, Respond_To_Message, 0, NULL, LOW_PRIORITY );

  fflush(stdout);
  E_handle_events();
  
  
}



static void Respond_To_Message() {
  
  //static  char  mess[MAX_MESSLEN];
  int   service_type;
  char  sender[MAX_GROUP_NAME];
  int   num_groups;
  char  target_groups[NUM_SERVERS][MAX_GROUP_NAME];
  int16 mess_type;
  int   endian_mismatch;


  /*
  int ret = SP_receive(Mbox, &service_type, sender, 100, &num_groups, target_groups,
             &mess_type, &endian_mismatch, sizeof(mess), mess);

  printf("this is ret: %d\n", ret);
  printf("%d\n", service_type);


  if ( Is_caused_join_mess(service_type)) {
    printf("we have received a membership message\n");
  }

  
  ret = SP_receive(Mbox, &service_type, sender, 100, &num_groups, target_groups,
                       &mess_type, &endian_mismatch, sizeof(mess), mess);

  printf("this is ret: %d\n", ret);
  printf("%d\n", service_type);

  if  (Is_caused_join_mess(service_type)) {
    printf("we have received a membership message\n");
  }
  */
  //  InfoForServer *info = malloc(sizeof(InfoForServer));
  //ret = SP_receive(Mbox, &service_type, sender, 100, &num_groups, target_groups,
  //               &mess_type, &endian_mismatch, sizeof(InfoForServer), (char*)info);

  //printf("This is ret:  %d\n", ret);
  // printf("This is mess: %s\n", mess);
  // printf("this is ret: %d\n", ret);
  //printf("%d\n", service_type);
  //assert(info->type == 2);
  //printf("This is the username: %s\n", info->user_name);



  //IDEA: if we receive an update then we're still going to end up doing some of the things in
  //the large if else block thing, so maybe we should check if it's an update and then change
  //type to that value and then just make sure that each if else statement takes care of the
  //case where the update arrives before the user or email is actually created

  //maybe we create a separate method for creating a user when we get an email, read, or
  //delete for a user that has not been created


  
  char *tmp_buf = malloc(MAX_PACKET_LEN);
  ret = SP_receive(Mbox, &service_type, sender, 100, &num_groups, target_groups,
                   &mess_type, &endian_mismatch, MAX_PACKET_LEN, (char*)tmp_buf);

  if (Is_caused_join_mess(service_type)) {
    //int num = atoi(&(target_groups[0][strlen(target_groups[0]) - 1]));
    //printf("num in caused by join: %d\n", num);
    for (int i = 0; i < num_groups; i++) {
      printf("%s\n", target_groups[i]);
    }   
    //servers_in_partition[num] = true;
    //num_servers_in_partition++;
    //printf("this is num servers in partition: %d\n", num_servers_in_partition);
    for (int i = 0; i < num_groups; i++) {
      int num = atoi(&(target_groups[i][strlen(target_groups[i]) - 1]));
      servers_in_partition[num] = true;
      num_servers_in_partition++;
    }
    printf("printing boolean array: \n");
    for(int i = 0; i < NUM_SERVERS; i++) {
      printf("%d ", servers_in_partition[i]);
    }
    printf("\n"); 
    return;
  } else if (Is_caused_leave_mess(service_type)) {
    int num = atoi(&(target_groups[0][strlen(target_groups[0]) - 1]));
    printf("num in caused by leave: %d\n", num);
    for (int i = 0; i < num_groups; i++) {
      printf("%s\n", target_groups[i]);
    }   
    servers_in_partition[num] = false;
    num_servers_in_partition--;
    printf("printing boolean array: \n");
    for (int i = 0; i < NUM_SERVERS; i++) {
      printf("%d ", servers_in_partition[i]);
    }
    printf("\n");
    return;
  }

  if (Is_caused_network_mess(service_type)) {
    printf("change in membership has occured!\n");
    for (int i = 0; i < NUM_SERVERS; i++) {
      servers_in_partition[i] = 0;
    }
    num_servers_in_partition = 0;
    //change in membership has occured
    for (int i = 0; i < num_groups; i++) {
      int num = atoi(&(target_groups[i][strlen(target_groups[i]) - 1]));
      servers_in_partition[num] = true;
      num_servers_in_partition++;
    }
    for (int i = 0; i < num_groups; i++) {
      printf("%s\n", target_groups[i]);
    }   
  }

  
  /*
  if (Is_reg_memb_mess(service_type)) {
    printf("\nPrinting groups (mem message received):\n");
    for (int i = 0; i < num_groups; i++) {
      printf("%s\n", target_groups[i]);
    }
    return;
    }*//* else if (Is_caused_join_mess(service_type) || Is_caused_leave_mess(service_type)) {
    printf("received either join or leave\n");
    return;
  }
  */
  
  int *type = (int*) tmp_buf;

  if (*type == 2) { 
    InfoForServer *info;
    info = (InfoForServer*) tmp_buf;
    printf("This is the username: %s\n", info->user_name);    
    printf("type: %d\n", *type);
    bool created_new_user = create_user_if_nonexistent(info->user_name);
    printf("about to print list\n and this is user list size: %d\n", users_list.num_nodes);
    print_list(&users_list, print_user);

    //now we want to send an update to all other machines IF A NEW USER WAS CREATED
    if (created_new_user) {
      //send update
      Update *to_be_sent = malloc(sizeof(Update));
      to_be_sent->type = 13;
      update_count++;
      to_be_sent->timestamp.counter = update_count;
      to_be_sent->timestamp.machine_index = my_machine_index;
      strcpy(to_be_sent->user_name, info->user_name);
      //copy our row of 2d array and send with update
      merge_matrix[my_machine_index][my_machine_index] = update_count;
      memcpy(to_be_sent->updates_array, merge_matrix[my_machine_index], sizeof(merge_matrix[my_machine_index]));
      printf("this is updates array: \n");
      for (int i = 0; i < NUM_SERVERS; i++) {
        printf("%d ", to_be_sent->updates_array[i]);
      }
      printf("\n");
      SP_multicast(Mbox, AGREED_MESS, group, 2, sizeof(Update), (char*)to_be_sent);
    }
    
  } else if (*type == 3) {

  } else if (*type == 4) {

  } else if (*type == 5) {

  } else if (*type == 6) {

  } else if (*type == 7) {

  } else if (*type == 10) {

  } else if (*type == 11) {

  } else if (*type == 12) {

  } else if (*type == 13) {
    Update *update;
    update = (Update*) tmp_buf;
    printf("we have received an update for a new user with name: %s\n", update->user_name);
    create_user_if_nonexistent(update->user_name);
    print_list(&users_list, print_user);
    //now we want to process the array that we received and update our own 2d array with the info
    memcpy(merge_matrix[update->timestamp.machine_index], update->updates_array, sizeof(update->updates_array));
    //consider if need to take max above or not; for now we say no
    merge_matrix[my_machine_index][update->timestamp.machine_index] = update->timestamp.counter;
    printf("printing merge matrix: \n");
    for (int i = 0; i < NUM_SERVERS; i++) {
      for (int j = 0; j < NUM_SERVERS; j++) {
        printf("%d ", merge_matrix[i][j]);
      }
      printf("\n");
    }
    
  } else if (*type == 20) {

  } else { //unknown type!
    printf("Unknown type. Exiting...\n");
    exit(1);
  }
  
}



bool create_user_if_nonexistent(char name[MAX_NAME_LEN]) {
  Node *temp = find(&users_list, &name[0], compare_users);
  if (temp == NULL) {
    //create new user
    User *user_to_insert = malloc(sizeof(User));
    strcpy(user_to_insert->name, name);
    printf("before creating email list for user\n");
    create_list(&(user_to_insert->email_list), sizeof(Email));
    add_to_end(&users_list, user_to_insert);
    printf("new user created!\n");
    return true;
  }
  return false;
}

int compare_users(void* user1, void* user2) {
  printf("entered compare users\n");
  User *user_in_linked_list = (User*) user1;
  if (user_in_linked_list->name == NULL) {
    printf("user is null\n");
  }
  char *user_search = (char*) user2;
  printf("before other print statements\n");
  printf("first one: %s\n",  (user_in_linked_list->name));
  printf("comparing %s and %s\n", (user_in_linked_list->name), user_search);
  printf("this is value for strcmp: %d\n", strcmp(user_in_linked_list->name, user_search));
  return strcmp(user_in_linked_list->name, user_search); 
}

void print_user(void *user) {
  User *temp = (User*) user;
  printf("Username: %s", temp->name);
  print_list(&(temp->email_list), print_email);
}

void print_email(void *email) {
  Email *temp = (Email*) email;
  printf("Timestamp: Counter: %d, Machine_index: %d, message_index: %d\n",
         temp->emailInfo.timestamp.counter, temp->emailInfo.timestamp.machine_index, temp->emailInfo.timestamp.message_index);
  printf("To: %s\n From: %s\n Subject: %s\n\n", temp->emailInfo.to_field, temp->emailInfo.from_field, temp->emailInfo.subject);
}

/*
static void Print_menu();
static void User_command();
static void Read_message();
static void Usage( int argc, char *argv[] );
static void Print_help();
static void Bye();
*/
/*
int main(int argc, char *argv[]) {
  int     ret;
  int     mver, miver, pver;
  sp_time test_timeout;

  test_timeout.sec = 5;
  test_timeout.usec = 0;

  Usage(argc, argv);

  if (!SP_version(&mver, &miver, &pver)) {
    printf("main: Illegal variables passed to SP_version()\n");
    Bye();
  }
  
  printf("Spread library version is %d.%d.%d\n", mver, miver, pver);

  ret = SP_connect_timeout(Spread_name, User, 0, 1, &Mbox, Private_group, test_timeout);
  if(ret != ACCEPT_SESSION) {
    SP_error(ret);
    Bye();
  }

  printf("User: connected to %s with private group %s\n", Spread_name, Private_group );

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

static void Read_message() {

  static  char     mess[MAX_MESSLEN];
          char     sender[MAX_GROUP_NAME];
          char     target_groups[MAX_MEMBERS][MAX_GROUP_NAME];
          membership_info  memb_info;
          vs_set_info      vssets[MAX_VSSETS];
          unsigned int     vsset_index;
          int              num_vs_sets;
          char             members[MAX_MEMBERS][MAX_GROUP_NAME];
          int    num_groups;
          int    service_type;
          int16    mess_type;
          int    endian_mismatch;
          int    i,j;
          int    ret;

  service_type = 0;

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
  printf("User> ");
  fflush(stdout);
}

static void Bye() {
  To_exit = 1;
  printf("\nBye.\n");
  SP_disconnect( Mbox );
  exit( 0 );
}
*/

/*


int compareEmail(void* temp1, void* temp2);
void printEmail(void *n);

int main() {
  List list;

  //Create a linked list
  create_list(&list, sizeof(Email));

  Email *temp1 = malloc(sizeof(Email));
  temp1->emailInfo.timestamp.counter = 1;
  temp1->emailInfo.timestamp.machine_index = 0;
  temp1->emailInfo.timestamp.message_index = 1;

  add_to_end(&list, temp1);

  print_list(&list, printEmail);


  Email *temp2 = malloc(sizeof(Email));
  temp2->emailInfo.timestamp.counter = 2;
  temp2->emailInfo.timestamp.machine_index = 0;
  temp2->emailInfo.timestamp.message_index = 2;

  insert(&list, temp2, compareEmail);
  print_list(&list, printEmail);



  Email *temp3 = malloc(sizeof(Email));
  temp3->emailInfo.timestamp.counter = 4;
  temp3->emailInfo.timestamp.machine_index = 2;
  temp3->emailInfo.timestamp.message_index = 3;


  insert(&list, temp3, compareEmail);
  print_list(&list, printEmail);
  
  Email *temp4 = malloc(sizeof(Email));
  temp4->emailInfo.timestamp.counter = 3;
  temp4->emailInfo.timestamp.machine_index = 2;
  temp4->emailInfo.timestamp.message_index = 4;

  insert(&list, temp4, compareEmail);
  print_list(&list, printEmail);

  Email *temp5 = malloc(sizeof(Email));
  temp5->emailInfo.timestamp.counter = 3;
  temp5->emailInfo.timestamp.machine_index = 0;
  temp5->emailInfo.timestamp.message_index = 100;

  insert(&list, temp5, compareEmail);
  print_list(&list, printEmail);
  
}



int compareEmail(void* temp1, void* temp2) {
  Email *one = (Email*) temp1;
  Email *two = (Email*) temp2;
  
  if (one->emailInfo.timestamp.counter < two->emailInfo.timestamp.counter) {
    return -1;
  } else if (one->emailInfo.timestamp.counter > two->emailInfo.timestamp.counter) {
    return 1;
  } else {
    //they are equal; use machine index for tie breaker
    if (one->emailInfo.timestamp.machine_index < two->emailInfo.timestamp.machine_index) {
      return -1;
    } else if(one->emailInfo.timestamp.machine_index > two->emailInfo.timestamp.machine_index) {
      return 1;
    } else {
      printf("Error: should not be here\n");
      return 0;
    }
  }
  
}

void printEmail(void *n) {
  Email *e = (Email*)n;
  printf("counter is: %d, machine index is: %d, message index is: %d\n",
         e->emailInfo.timestamp.counter,  e->emailInfo.timestamp.machine_index,  e->emailInfo.timestamp.message_index);  
}
*/
