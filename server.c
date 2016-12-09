/*
Sarah Sukardi
Mariya Kazachkova
Distributed Systems: Final Project
December 9, 2016
*/

#include "sp.h"
#include "net_include.h"


#define int32u unsigned int

//Variables for Spread
static char         Server[80];
static char         Spread_name[80];

static char         Private_group[MAX_GROUP_NAME];
static mailbox      Mbox;
//static int          Num_sent;

static char         group[80] = "ssukard1mkazach1_2"; 
static char         server_own_group[80] = "ssukard1mkazach1_server_";
//static char         extra_group[80] = "extra_group_ss_mk";       

static int          To_exit = 0;

int                 ret;
int                 mver, miver, pver;
sp_time             test_timeout;


// Our own variables
List                users_list;
List                array_of_updates_list[NUM_SERVERS];
int                 my_machine_index;
int                 merge_matrix[NUM_SERVERS][NUM_SERVERS];
int                 update_index = 0;
bool                servers_in_partition[NUM_SERVERS] = { false };
int                 num_servers_in_partition = 0; //THIS MAY NOT BE CORRECT SO DO NOT USE
int                 lamport_counter = 0;
int                 num_emails_checked = 0;
int                 min_seen_global = -1;
int                 global_counter = 0;
int                 min_global = 0;
int                 num_updates_received = 0;

// Global variables used for sending header to client
int                 message_number_stamp = 1;
int                 num_headers_added;
HeaderForClient     *client_header_response;     

char                sender[MAX_GROUP_NAME];



//fucking variables for fucking flow control
int min_update_global[NUM_SERVERS] = { -1 };
int max_update_global[NUM_SERVERS] = { -1 };
int num_updates_sent_so_far[NUM_SERVERS] = { 0 };
bool done_sending_for_merge[NUM_SERVERS] = { true };
int current_i = -1; //this one expecially makes me think this flow control idea is dumb
int num_updates_received_from_myself = 0;
int who_sends[NUM_SERVERS] = { -1 };

//Our own methods 
static void   Respond_To_Message();
static void   Bye();
static int    compare_users(void *user1, void *user2);
static int    compare_email(void *temp, void *temp2);
static int    compare_update(void* update1, void* update2);
static int    compare_email_for_find(void* email1, void* email2);
static bool   create_user_if_nonexistent(char *name);
static void   print_user(void *user);
static void   print_email(void *email);
static void   print_update(void *update);
static bool   send_updates_for_merge(void *update);
static void   add_to_struct_to_send(void *data);
static void   add_to_header(Email *email);
static bool   can_delete_update(void* update);
static int    max(int one, int two);
static int    min(int one, int two);


// **************************** MAIN METHOD **************************** //
int main(int argc, char *argv[]) {

  //Error check input
  if (argc <= 1) {
    printf("Please input the server number (1-5) as a command-line input.\n");
    exit(0);
  }
  
  //Create a linked list of users
  create_list(&users_list, sizeof(User));

  //initialize array of linked list of updates
  for (int i = 0; i < NUM_SERVERS; i++) {
    create_list(&array_of_updates_list[i], sizeof(Update));
  }

  //Populate entire merge_matrix with 0's 
  for (int i = 0; i < NUM_SERVERS; i++) {
    for (int j = 0; j < NUM_SERVERS; j++) {
      merge_matrix[i][j] = 0;
    }
  }

  //Get the machine index from command-line input
  my_machine_index =  atoi(argv[1]) - 1; //subtract 1 for correct indexing

  //Concatenate with server_own_group ("ssukard1_makazach1_server_##")
  char my_machine_index_str[20];  
  sprintf(my_machine_index_str, "%d", my_machine_index + 10);
  strcat(server_own_group, my_machine_index_str);
  printf("%s\n", server_own_group);

  //Spread setup (copypasted from class_user sample code)
  sprintf(Server, "user_mk_ss");
  sprintf(Spread_name, "10160");

  if (!SP_version(&mver, &miver, &pver)) {
    printf("main: Illegal variables passed to SP_version()\n");
    Bye();
  }

  printf("Spread library version is %d.%d.%d\n", mver, miver, pver);

  ret = SP_connect_timeout(Spread_name, Server, 0, 1, &Mbox, Private_group, test_timeout);
  if (ret != ACCEPT_SESSION) {
    SP_error(ret);
    Bye();
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

  printf("Connected!\n");

  //Using E_attach is similar to putting the function in th 3rd argument in an infinite for loop
  E_init(); 
  E_attach_fd(Mbox, READ_FD, Respond_To_Message, 0, NULL, LOW_PRIORITY);

  fflush(stdout);

  //Start the "infinite for loop"
  E_handle_events();
}


//Primary method that is triggered whenever a message (from another server or client) is received
static void Respond_To_Message() {

  //necessary Variables
  int   service_type;
  int   num_groups;
  char  target_groups[NUM_SERVERS][MAX_GROUP_NAME];
  int16 mess_type;
  int   endian_mismatch;

  //IDEA: if we receive an update then we're still going to end up doing some of the things in
  //the large if else block thing, so maybe we should check if it's an update and then change
  //type to that value and then just make sure that each if else statement takes care of the
  //case where the update arrives before the user or email is actually created

  //maybe we create a separate method for creating a user when we get an email, read, or
  //delete for a user that has not been created
  
  
  // Putting whatever was received into tmp_buf, which will be cast into whatever type
  // it is by looking at the first digit!
  char *tmp_buf = malloc(MAX_PACKET_LEN);
  ret = SP_receive(Mbox, &service_type, sender, 100, &num_groups, target_groups,
                      &mess_type, &endian_mismatch, MAX_PACKET_LEN, (char*)tmp_buf);

  printf("\nATTN: MESSAGE RECEIVED WITH SERVICE TYPE: %d From Sender: %s\n", service_type, sender);

  // **************************** PARSE MEMBERSHIP MESSAGES **************************** //

  //If transition message, do nothing
  if (Is_transition_mess(service_type)) {
    //  printf("Transition message received!\n");
    return;
  }

  //Parsing the server entering message (Should only be executed at the beginning of the program)
  if (Is_caused_join_mess(service_type)) {
    // printf("A join message was received. Here are the contents of target_groups:\n");
    //for (int i = 0; i < num_groups; i++) {
    //printf("%s\n", target_groups[i]);
    //}   

    //printf("this is sender: %s", sender);
    char first_char = sender[0];
    //printf("this is the first first char. of sender: %d\n", first_char);

    //If number received, then a NON-server group was joined by a client.
    if (first_char >= '0' && first_char <= '9') {
      //printf("Received join message from non-server.\n");
      return; //this means client has joined the group you've already joined. Do nothing
    }
    
    //Otherwise, it is a server group.
    //printf("Received join message from server.\n");

    //Change servers_in_partition array to reflect current servers in array
    for (int i = 0; i < num_groups; i++) {
      int num = atoi(&(target_groups[i][strlen(target_groups[i]) - 1]));
      //printf("Adding server with name %s into index %d...\n", target_groups[i], num);
      servers_in_partition[num] = true;
      num_servers_in_partition++;
    }

    //Print the servers_in_partition array    
    printf("printing boolean servers_in_partition array: \n");
    for(int i = 0; i < NUM_SERVERS; i++) {
      printf("%d ", servers_in_partition[i]);
    }
    printf("\n"); 
    return;

  } else if (Is_caused_leave_mess(service_type) || Is_caused_disconnect_mess(service_type)) {
    //If server gets a leave message from a single client-server spread group, LEAVE THAT GROUP ALSO.
    //printf("this is sender: %s", sender);
    char first_char = sender[0];
    //printf("this is char first char %d\n", first_char);

    //Received leave message from non-client server
    if (first_char >= '0' && first_char <= '9') {
      //printf("Received %s message from non-server\n", Is_caused_disconnect_mess(service_type) ? "disconnect" : "leave");
      SP_leave(Mbox, sender);
      return; //this means client has left group between client and server; also leave client group
    }

    return;
    /*
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
    return; */
  }


  //************************** RECONCILIATION PROCESS *************************//
  //This method is triggered whenever there is a change in membership detected!
  if (Is_caused_network_mess(service_type)) {     
    //    num_matrices_received = 0;


    int number_of_groups = num_groups;
    //printf("A change in membership has occurred!\n");

    //If server gets a leave message from a single client-server spread group, LEAVE THAT GROUP ALSO.
    //printf("this is sender: %s", sender);
    char first_char = sender[0];
    //printf("this is char first char %d\n", first_char);

    //If number received, then a NON-server group was partitioned
    if (first_char >= '0' && first_char <= '9') {
      printf("Received network message from non-server.\n");
      //printf("this is sender of non-server thing: %s\n", sender);
      //printf("this is target_groups: %s\n", target_groups[0]);
      SP_leave(Mbox, sender);
      printf("Server has received that the client has been disconnected from the client-server group due to partition. Server has also disconnected\n");
      //this means client has LEFT the group that ONLY you two are in. Do NOT do reconciliation process!!!
      //I don''t think it means it joined ^^^ I think it means that it left...when it joins not a network message, right?
      //yea you're right lol i must have copypasted that from somewhere :P (made fixes in comment)
      return; 

    }


    //printf("\nTHIS IS NUM GROUPS after receiving network message %d\n", num_groups);
    //Should we leave the group that the client and server are in in the above process ^^
    //so that the group isn't rejoined when the partition no longer persists
    
    printf("Entering reconciliation process\n");
    //Set all servers in partition to false
    for (int i = 0; i < NUM_SERVERS; i++) {
      servers_in_partition[i] = false;
    }
    num_servers_in_partition = 0;


    //IN THE EVENT OF CASCADING MERGES
    for (int i = 0; i < NUM_SERVERS; i++) {
      Update *t = get_tail(&(array_of_updates_list[i]));
      if (t != NULL) {
        merge_matrix[my_machine_index][i] = t->timestamp.message_index;
      }
    }

    
    //change in membership has occured
    //populate servers_in_partition array with the new servers in the partition
    for (int i = 0; i < num_groups; i++) {
      int num = atoi(&(target_groups[i][strlen(target_groups[i]) - 1]));
      servers_in_partition[num] = true;
      num_servers_in_partition++;
    }

    //Print all values in target_group (all the members in our partition) -- for debug
    //for (int i = 0; i < num_groups; i++) {
    //printf("%s\n", target_groups[i]);
    //}

    //Allocate space for and populate MergeMatrix sent to other servers
    MergeMatrix *merge = malloc(sizeof(MergeMatrix));
    merge->type = 20;
    merge->machine_index = my_machine_index;

    //Copy local merge matrix into MergeMatrix to be sent
    for (int i = 0; i < NUM_SERVERS; i++) {
      memcpy(merge->matrix[i], merge_matrix[i], sizeof(merge_matrix[i]));
    }

    //Print merge matrix sent (for debug)
    
    //printf("\n\n\n/******** MERGE MATRIX BEFORE RECONCILIATION BEGUN: ********/\n\n\n");    
    /*for (int i = 0; i < NUM_SERVERS; i++) {
      for (int j = 0; j < NUM_SERVERS; j++) {
        printf("%d ", merge->matrix[i][j]);
      }
      printf("\n");
    }*/

    //send 2d MergeMatrix array to everyone else
    SP_multicast(Mbox, AGREED_MESS, group, 2, sizeof(MergeMatrix), (char*)merge);
    
    //Free to prevent memory leaks
    free(merge);
    merge = NULL;

    //receive num_groups number of 2D arrays
    int num_matrices_received = 0;
    int min_array[NUM_SERVERS][NUM_SERVERS];

    //fill min_array with 0 initially
    for (int i = 0; i < NUM_SERVERS; i++) {
      for (int j = 0; j < NUM_SERVERS; j++) {
        min_array[i][j] = 0;
      }
    }
    
    //Iterate until all (num_groups number of) merge matrices have been processed
    for (;;) {
      //Allocate space for a MergeMatrix and receive it from sender

      MergeMatrix *merge = malloc(sizeof(MergeMatrix));
      SP_receive(Mbox, &service_type, sender, 100, &num_groups, target_groups,
                 &mess_type, &endian_mismatch, sizeof(MergeMatrix), (char*)merge);

      
      char first_char = sender[0];
      //printf("this is char first char %d\n", first_char);
      
      //If number received, then a NON-server group was partitioned
      if (first_char >= '0' && first_char <= '9') {
        //printf("message received from nonserver during merge matrix. This is service type: %d", service_type);
        //printf("Received network message from non-server.\n");
        //printf("this is sender of non-server thing: %s\n", sender);
        //printf("this is target_groups: %s\n", target_groups[0]);
        // SP_leave(Mbox, sender);
        //printf("Server has received that the client has been disconnected from the client-server group due to partition. Server has also disconnected\n");
        //this means client has LEFT the group that ONLY you two are in. Do NOT do reconciliation process!!!
        //I don''t think it means it joined ^^^ I think it means that it left...when it joins not a network message, right?
        //yea you're right lol i must have copypasted that from somewhere :P (made fixes in comment)
        continue; 
      }
      
      num_matrices_received++;

      //printf("*************this is number groups inside of the for loop: %d\n", num_groups);

      
      printf("\n\nMERGE MATRIX RECEIVED from server: %d\n", merge->machine_index);
      //for (int i = 0; i < NUM_SERVERS; i++) {
      //for (int j = 0; j < NUM_SERVERS; j++) {
      //  printf("%d ", merge->matrix[i][j]);
      //}
      //printf("\n");
      //}
      
      //Process MergeMatrix
      for (int i = 0; i < NUM_SERVERS; i++) {
        for (int j = 0; j < NUM_SERVERS; j++) {
          // populate the min_array (min seen in all merge matrices sent)
          min_array[i][j] = min(merge_matrix[i][j], merge->matrix[i][j]);
          // update the local merge_matrix (max seen in all merge matrices sent)
          merge_matrix[i][j] = max(merge_matrix[i][j], merge->matrix[i][j]);
        }
      }
      free(merge);
      merge = NULL;
      //Exit loop once num_groups merge matrices have been received
      if (num_matrices_received == number_of_groups) {
        break;
      }
    }


    //now both (current merge_matrix and min_array) arrays loaded
    //it's time to figure out which machine needs to send updates
    //int who_sends[NUM_SERVERS] = { -1 }; //array of who must send updates
    int max_seen = 0;
    printf("This is servers in partition array: \n");

    //resetting since now global
    for (int i = 0; i < NUM_SERVERS; i++) {
      who_sends[i] = -1;
    }


    
    //For debug: print servers in our new partition
    //for (int i = 0; i < NUM_SERVERS; i++) {
    //printf("%d ", servers_in_partition[i]);
    //}

    //Find which server has the maximum
    for (int i = 0; i < NUM_SERVERS; i++) {
      for (int j = 0; j < NUM_SERVERS; j++) {
        if (servers_in_partition[j]) {
          //If a greater value seen, set the value of max_seen, as well as
          //update who must send values in that column then
          if (merge_matrix[j][i] > max_seen) {            
            max_seen = merge_matrix[j][i];
            who_sends[i] = j;
          }
        }
      }
      for (int k = 0; k < NUM_SERVERS; k++) {
        if (servers_in_partition[k]) {
          merge_matrix[k][i] = max_seen;
        }
      }
      //reset max_seen back to 0
      max_seen = 0;
    }


    /*
      The above code is the issue with doing cascading merges. Since we update to max seen under the assumption
      there is a chance that each process won't get all of the updates that it says it has....
      what if...before we start the reconcililng thing we change our row in the merge matrix with the 
      number that we have up to for each index in our updates array. That way when we reconcile each process
      knows a correct representation of what updates other processes have
     */



    
    //For debug: Print the who sends array to see who is sending what updates
    printf("printing who sends array!\n");
    for (int i = 0; i < NUM_SERVERS; i++) {
      //assert(who_sends[i] >= 0);
      printf("%d ", who_sends[i]);
    }

    printf("\n");
    
    //Do the actual sending of messages
    int min_seen = INT_MAX;
    for (int i = 0; i < NUM_SERVERS; i++) {
      min_seen = INT_MAX;

      //If who_sends[i] is our machine index, WE must send      
      if (who_sends[i] == my_machine_index) {        
        //Find the min for the process we are sending for and send from there until the end
        for (int j = 0; j < NUM_SERVERS; j++) {
          if (min_array[j][i] < min_seen) {
            min_seen = min_array[j][i];
          }
        }
        min_update_global[i] = min_seen; //where we start from
        Update *last = get_tail(&(array_of_updates_list[i]));
        max_update_global[i] = last->timestamp.message_index; //where we end
        done_sending_for_merge[i] = false; //so that we know we are still sending
        current_i = i; //so that we know which index to use when using our NUMEROUS global arrays (fuck this, again)
        
        //Set the min_seen_global variable to the minimum we have seen 
        //(global only set here for safety)
        min_seen_global = min_seen;

        // now we start to send
        bool done  = forward_iterator(&(array_of_updates_list[i]), send_updates_for_merge);
        done_sending_for_merge[i] = done; 
      }

    }

    /*
    
    //delete things in updates array that we no longer need
    int min = INT_MAX;
    bool can_move_forward = true;
    for (int i = 0; i < NUM_SERVERS; i++) {
      for (int j = 0; j < NUM_SERVERS; j++) {
        if (merge_matrix[j][i] < min) {
          min = merge_matrix[j][i];
        }
      }
      min_global = min;
      //for index i in updates array delete up until and including the update with the min value
      can_move_forward = true;
      while(can_move_forward) {
        can_move_forward = remove_from_beginning(&(array_of_updates_list[i]), can_delete_update);
      }
    }

    */

    
    //Print merge matrix sent (for debug)
    printf("\n\n\n/******** MERGE MATRIX AFTER RECONCILIATION COMPLETED: ********/\n\n\n");
    for (int i = 0; i < NUM_SERVERS; i++) {
      for (int j = 0; j < NUM_SERVERS; j++) {
        printf("%d ", merge_matrix[i][j]);
      }
      printf("\n");
    }


    return;
  }


  // **************************** PARSE NON-MEMBERSHIP MESSAGES **************************** //
  //Cast first digit into integer to find out the type  
  int *type = (int*) tmp_buf;

  if (*type >= 10 && *type <= 13 ) {
    Update *temp = (Update*) tmp_buf;
    Update *existing_update = find(&(array_of_updates_list[temp->timestamp.machine_index]),(void*)temp, compare_update);
    if (existing_update == NULL) {
      num_updates_received++;                   
    }
    if (temp->index_of_machine_resending_update == my_machine_index) { //have received an update from ourselves that we resent
      printf("\n\nHAVE RECEIVED AN UPDATE THAT WE RESENT!!!!!\\n\n");
      num_updates_received_from_myself++;
    }
  }


   if (num_updates_received_from_myself % 10 == 0) {
    //time to keep on sending. boo
     for(int i = 0; i < NUM_SERVERS; i++) {
       if(who_sends[i] == my_machine_index) { //one of the indexes i'm responsible for; keep sending
         if (!done_sending_for_merge[i]) { //means we have more to send for this machine
           current_i = i; //set our global variable
           num_updates_sent_so_far[i] = 0; //set this back to zero since it will be incrmemented in interator method
           //bool more_to_send  = forward_iterator(&(array_of_updates_list[i]), send_updates_for_merge);
           //done_sending_for_merge[i] = !more_to_send;
           bool done  = forward_iterator(&(array_of_updates_list[i]), send_updates_for_merge);
           done_sending_for_merge[i] = done;
         }
       }
     }
     
   }

  
  if (num_updates_received >= NUM_FOR_DELETE_UPDATES) {
    num_updates_received = 0;
    //delete things in updates array that we no longer need
    int min = INT_MAX;
    bool can_move_forward = true;
    for (int i = 0; i < NUM_SERVERS; i++) {
      for (int j = 0; j < NUM_SERVERS; j++) {
        if (merge_matrix[j][i] < min) {
          min = merge_matrix[j][i];
        }
      }
      min_global = min;
      //for index i in updates array delete up until and including the update with the min value
      can_move_forward = true;
      while(can_move_forward) {
        can_move_forward = remove_from_beginning(&(array_of_updates_list[i]), can_delete_update);
      }
    }

  }
  
  ///////////////////////////////// parse messages from client ////////////////////////////////
  //If *type is of type 1-7, we have RECEIVED A MESSAGE FROM THE CLIENT.

  if (*type == 1) {
    InfoForServer *info = (InfoForServer*) tmp_buf;
    //printf("This is the username: %s\n", info->user_name);    
    //printf("type: %d\n", *type);
    bool created_new_user = create_user_if_nonexistent(info->user_name);

    //Only send update to other servers if new user was created
    if (created_new_user) {      
      //Dynamically create and send update
      Update *new_user_update = malloc(sizeof(Update));
 
     //Fill in relevant parameters
      new_user_update->type = 13;
      update_index++;
      new_user_update->timestamp.message_index = update_index;
      new_user_update->timestamp.machine_index = my_machine_index;
      strcpy(new_user_update->user_name, info->user_name);
           
      //Send the Update to ALL OTHER SERVERS in the same partition
      SP_multicast(Mbox, AGREED_MESS, group, 2, sizeof(Update), (char*)new_user_update);
      free(new_user_update);
    }

  } else if (*type == 2) { // We received a "new user" message from the client

    //We know that the message received was of type InfoForServer, so we can cast it accordingly
    InfoForServer *info = (InfoForServer*) tmp_buf;
    //printf("This is the username: %s\n", info->user_name);    
    //printf("type: %d\n", *type);
    bool created_new_user = create_user_if_nonexistent(info->user_name);

    // Send an InfoForClient object back to client with a unique group name saying that 
    // the connection was successfully established
    InfoForClient *info_client = malloc(sizeof(InfoForClient));
    info_client->type = 4;
    char group_for_client[MAX_GROUP_NAME];

    //Generate group name from current time to guarantee uniqueness
    int seconds = time(NULL); //get current time
    sprintf(group_for_client, "%d", seconds);

    //Join group and send to client
    SP_join(Mbox, group_for_client); 
    strcpy(info_client->client_server_group_name, group_for_client);
    //printf("this is client server group name: %s\n",info_client->client_server_group_name);

    //Send message back to client confirming the connection occurred
    SP_multicast(Mbox, AGREED_MESS, sender, 2, sizeof(InfoForClient), (char*)info_client);
    free(info_client);
    
    //Only send update to other servers if a new user was created
    if (created_new_user) {      
      //Dynamically create and send update
      Update *to_be_sent = malloc(sizeof(Update));
 
     //Fill in relevant parameters
      to_be_sent->type = 13;
      update_index++;
      to_be_sent->timestamp.message_index = update_index;
      to_be_sent->timestamp.machine_index = my_machine_index;
      strcpy(to_be_sent->user_name, info->user_name);
           
      //Send the Update to ALL OTHER SERVERS in the same partition
      SP_multicast(Mbox, AGREED_MESS, group, 2, sizeof(Update), (char*)to_be_sent);
    }

    
  } else if (*type == 3) { //We received a "list headers" message from the client
    //We know that the thing that was sent was of type InfoForServer, so we can cast it accordingly
    InfoForServer *info = (InfoForServer*) tmp_buf;
    User *user = find(&(users_list), info->user_name, compare_users);
    
    client_header_response = malloc(sizeof(HeaderForClient));
    client_header_response->type = 1;

    //Initializing stuff for backwards iterator
    message_number_stamp = 1;
    num_headers_added = 0;
    client_header_response->done = false;
    backward_iterator(&(user->email_list), add_to_struct_to_send);

    //send the very last few messages, initializing remaining slots on header to have 
    //a message_number of -1
    for (int i = num_headers_added; i < 10; i++) {
      client_header_response->headers[i].message_number = -1;
    }
    client_header_response->done = true;
    SP_multicast(Mbox, AGREED_MESS, sender, 2, sizeof(HeaderForClient), (char*)client_header_response);
    free(client_header_response);
    client_header_response = NULL;
  
  } else if (*type == 4) { // Client put in request to server to mail message to another user

    //NOTE: We are NOT directly putting the email in our own personal email list (in our Users linked list)
    //because we will RECEIVE THIS MESSAGE FROM OURSELVES-- THEN we will put it in our own personal email list.

    //We know that the thing that was sent was of type InfoForServer, so we can cast it accordingly
    InfoForServer *info = (InfoForServer *) tmp_buf;    

    //Send an update to all other servers to put that email into their email list
    Update *to_be_sent = malloc(sizeof(Update));
    to_be_sent->type = 10;
    update_index++;
    to_be_sent->timestamp.counter = -1; // this does not matter
    to_be_sent->timestamp.machine_index = my_machine_index;
    to_be_sent->timestamp.message_index = update_index;

    lamport_counter++;

    to_be_sent->email = info->email;
    to_be_sent->email.emailInfo.timestamp.machine_index = my_machine_index;
    to_be_sent->email.emailInfo.timestamp.counter = lamport_counter;
    to_be_sent->email.emailInfo.timestamp.message_index = -1; //Message index attached to an UPDATE'S timestamp is never used

    to_be_sent->email.exists = true;
    to_be_sent->email.deleted = false;
    to_be_sent->email.read = false;
    //copy our row of the 2d array and send with update 
    //merge_matrix[my_machine_index][my_machine_index] = update_index;
    //memcpy(to_be_sent->updates_array, merge_matrix[my_machine_index], sizeof(merge_matrix[my_machine_index]));
    
    //Print the contents of the email that was received for debugging purposes
    //printf("Sending an email update to other servers! \nHere's the Email:\n");
    //printf("TO: %s\n", to_be_sent->email.emailInfo.to_field);
    //printf("FROM: %s\n", to_be_sent->email.emailInfo.from_field);
    //printf("SUBJECT: %s\n", to_be_sent->email.emailInfo.subject);
    //printf("MESSAGE: %s\n", to_be_sent->email.emailInfo.message);
    
    //Send the Update to ALL OTHER SERVERS in the same partition
    SP_multicast(Mbox, AGREED_MESS, group, 2, sizeof(Update), (char*)to_be_sent);
    
  } else if (*type == 5) { //Server received a "DELETE MESSAGE" message from the client
  
    //We know that the thing that was sent was of type InfoForServer, so we can cast it accordingly
    InfoForServer *info = (InfoForServer *) tmp_buf;    

    //Send an update to all other servers to mark that email as read
    Update *to_be_sent = malloc(sizeof(Update));
    to_be_sent->type = 12;
    update_index++;
    to_be_sent->timestamp.message_index = update_index;
    to_be_sent->timestamp.machine_index = my_machine_index;
    to_be_sent->timestamp_of_email = info->message_to_delete;    
    strcpy(to_be_sent->user_name, info->user_name);

    //Find user that we are logged into
    // User *user = find(&users_list, (void*)info->user_name, compare_users);
    // assert(user != NULL); //for debug
    
    /*
    //set global variable back equal to zero
    num_emails_checked = 0;
    Email *email = find_backwards(&(user->email_list), (void*)&(info->message_to_delete), compare_email_for_find);

    if (email == NULL) {
      //printf("Error: should not be null yet!\n");
      //CHECK IF NULL AND SEND ERROR MESSAGE BACK TO USER
      //do we actually need o do this or can we leave it the way it is??
      return;
    }

    TimeStamp timestamp = email->emailInfo.timestamp;
    //printf("THIS IS THE EMAIL TIMESTAMP COUNTER: %d\n\n", timestamp.counter);
    */
    
    //Send the Update to ALL OTHER SERVERS in the same partition
    SP_multicast(Mbox, AGREED_MESS, group, 2, sizeof(Update), (char*)to_be_sent);

  } else if (*type == 6) { //Server received a "READ MESSAGE" message from the client

    //We know that the thing that was sent was of type InfoForServer, so we can cast it accordingly
    InfoForServer *info = (InfoForServer *) tmp_buf;    

    //Send an update to all other servers to mark that email as read
    Update *to_be_sent = malloc(sizeof(Update));
    to_be_sent->type = 11;
    update_index++;
    to_be_sent->timestamp.message_index = update_index;
    to_be_sent->timestamp.machine_index = my_machine_index;
    to_be_sent->timestamp_of_email = info->message_to_read;    
    strcpy(to_be_sent->user_name, info->user_name);

    //Send the Update to ALL OTHER SERVERS in the same partition
    SP_multicast(Mbox, AGREED_MESS, group, 2, sizeof(Update), (char*)to_be_sent);

    //Find user that we are logged into
    User *user = find(&users_list, (void*)info->user_name, compare_users);
    assert(user != NULL); //for debug
    
    /*
    //set global variable back equal to zero
    num_emails_checked = 0;
    //printf("this is user's name: %s\n", user->name);
    //printf("this is user email list:\n");
    print_list(&(user->email_list), print_email);
    //printf("this is message to read: %d\n", info->message_to_read);
    Email *email = find_backwards(&(user->email_list), (void*)&(info->message_to_read), compare_email_for_find);
    
    if (email == NULL) {
      //printf("Error: should not be null yet!\n");
      //CHECK IF NULL AND SEND ERROR MESSAGE BACK TO USER

      return;
    } 

    TimeStamp timestamp = email->emailInfo.timestamp;
    //printf("THIS IS THE EMAIL TIMESTAMP COUNTER: %d\n\n", timestamp.counter);
    */

    //TODO: send the actual email body back to client
    InfoForClient *info_for_client = malloc(sizeof(InfoForClient));
    info_for_client->type = 2; // this means that an email has been sent back for the client to read

    Email *dummy = malloc(sizeof(Email));
    dummy->emailInfo.timestamp = info->message_to_read;
  
    Email *email_to_read = find_backwards(&(user->email_list), (void*)dummy, compare_email);
    free(dummy);

    info_for_client->email = *email_to_read;
    
    SP_multicast(Mbox, AGREED_MESS, sender, 2, sizeof(InfoForClient), (char*)info_for_client);

    
  } else if (*type == 7) { //Server received a "PRINT MEMBERSHIP" message from the client 

    //We know that the thing that was sent was of type InfoForServer, so we can cast it accordingly
    InfoForClient *info_for_client = malloc(sizeof(InfoForClient));
    info_for_client->type = 3; // this means that we are sending membership information

    for (int i = 0; i < NUM_SERVERS; i++) {
      if (servers_in_partition[i]) { //if the ith server is in our partition, add it to memb_identities
        char my_machine_index_str[20];  
        sprintf(my_machine_index_str, "%d", i + 10);

        char name_to_concat_with[80] = "ssukard1mkazach1_server_";
        strcat(name_to_concat_with, my_machine_index_str);

        strcpy(info_for_client->memb_identities[i], name_to_concat_with);
      } else { //otherwise, copy the empty string in
        strcpy(info_for_client->memb_identities[i], "");
      }
    }

    SP_multicast(Mbox, AGREED_MESS, sender, 2, sizeof(InfoForClient), (char*)info_for_client);


  // **************************** parse messages from server **************************** //
  // If *type is of type 10-13, we have RECEIVED A MESSAGE FROM THE CLIENT.

  } else if (*type == 10) { //server received a NEW EMAIL update from another server
    //Cast into Update type
    Update *update = (Update*) tmp_buf;
    //printf("we have received an update for a new email!\n");
    create_user_if_nonexistent(update->email.emailInfo.to_field); //create new user if new user doesn't exist yet
    print_list(&users_list, print_user);

    //update our own lamport counter
    lamport_counter = max(lamport_counter, update->email.emailInfo.timestamp.counter);

    //TODO:updates_array needs to be taken into account; update our 2d array
    for (int i = 0; i < NUM_SERVERS; i++) {
      if (servers_in_partition[i]) {
        merge_matrix[i][update->timestamp.machine_index] = update->timestamp.message_index;
      }
    }

    /*
    printf("\nPrinting merge matrix: \n");
    for (int i = 0; i < NUM_SERVERS; i++) {
      for (int j = 0; j < NUM_SERVERS; j++) {
        printf("%d ", merge_matrix[i][j]);
      }
      printf("\n");
    }
    */

    //insert(&(array_of_updates_list[update->timestamp.machine_index]), (void*)update, compare_update);


    //this logic is used in the chance that we receive an update for an email that we already have; we ignore it!
    Update *existing_update = find(&(array_of_updates_list[update->timestamp.machine_index]),(void*)update, compare_update);
    //User *user = find(&(users_list), update->user_name, compare_users);
    //if (user == NULL) {
    //create_user_if_nonexistent(update->user_name);
    //}

    User *user_found = find(&(users_list), update->email.emailInfo.to_field, compare_users);
    assert(user_found != NULL); //should never be null because we just created it if it's null
    Email *temp = find(&(user_found->email_list), (void*)&(update->email), compare_email);
    if (temp != NULL) {
      //printf("Have received an update for an email that we already have! IGNORING!!!!!");
      //email exists so don't want to process it
      return;
    }
    
    if (existing_update != NULL) {
      //printf("Update already exists! Should only show up in a merge!!\n");
      return;
    } else {
      //update is new! We want to insert it into our array!!!!
      insert(&(array_of_updates_list[update->timestamp.machine_index]), (void*)update, compare_update);

      
      printf("Printing array of updates list: \n");
      for (int i = 0; i < NUM_SERVERS; i++) {
        printf("This is the linked list for index %d: \n", i);
        print_list(&(array_of_updates_list[i]), print_update);
      }
    
      //printf("this is subject: %s\n", update->email.emailInfo.subject);
    
      User *temp = find(&users_list, (void*)update->email.emailInfo.to_field, compare_users);
      assert(temp != NULL);
      //printf("User found! Here's their name: %s\n", temp->name);


      //printf("\nTHIS IS THE LAMPORT COUNTER THAT WE ARE GOING TO BE INSERTING: \n");
      //printf("This is counter: %d and message index: %d and machine index: %d\n",
      //     update->email.emailInfo.timestamp.counter, update->email.emailInfo.timestamp.message_index, update->email.emailInfo.timestamp.machine_index);

      insert(&(temp->email_list),(void*) &(update->email), compare_email);
      //printf("inserted into user's email. Now printing user's email inbox: \n");
      //print_list(&(temp->email_list), print_email);
    }

  } else if (*type == 11) { //server received a READ EMAIL update from another server
    //Cast into Update type
    Update *update = (Update*) tmp_buf;
    //printf("we have received an update for a new email!\n");

    //TODO: updates_array needs to be taken into account; update our 2d array
    for (int i = 0; i < NUM_SERVERS; i++) {
      if (servers_in_partition[i]) {
        merge_matrix[i][update->timestamp.machine_index] = update->timestamp.message_index;
      }
    }

    //insert(&(array_of_updates_list[update->timestamp.machine_index]), (void*)update, compare_update);

    Update *existing_update = find(&(array_of_updates_list[update->timestamp.machine_index]),(void*)update, compare_update);
    if (existing_update != NULL) {
      //printf("Update already exists! Should only show up in a merge!!\n");
      return;
    } else {
      //update is new! We want to insert it into our array!!!!
      insert(&(array_of_updates_list[update->timestamp.machine_index]), (void*)update, compare_update);
    
      printf("Printing array of updates list: \n");
      for (int i = 0; i < NUM_SERVERS; i++) {
        printf("This is the linked list for index %d: \n", i);
        print_list(&(array_of_updates_list[i]), print_update);
      }
    
      //printf("this is subject: %s\n", update->email.emailInfo.subject);

      /**
         How is the above code working (printing the subject)? Are we sending the whole email? Aren't we just sending the timestamp?
       **/

      /*CODE NOT TESTED YET*/
      create_user_if_nonexistent(update->user_name);
      //ideally this will create the new user if it has not been made yet
      
      User *temp = (User*) find(&users_list, (void*)update->user_name, compare_users);
      assert(temp != NULL);
      //printf("User found! Here's their name: %s\n", temp->name);

      printf("timestamp of email (just counter and machine index): %d %d\n", update->timestamp_of_email.counter, update->timestamp_of_email.machine_index);

      Email *dummy = malloc(sizeof(Email));
      dummy->emailInfo.timestamp = update->timestamp_of_email;
    
      Email *email = find_backwards(&(temp->email_list), (void*)dummy, compare_email);
      free(dummy);

      if (email == NULL) {
        printf("\n\nNEW EMAIL IS BEING CREATED!!\n\n");
        Email *new_email = malloc(sizeof(Email));
        new_email->emailInfo.timestamp = update->timestamp_of_email;
        new_email->read = true;
        new_email->exists = false;
        new_email->deleted = false;
            
        insert(&(temp->email_list),(void*)(new_email), compare_email);      
      } else {
        email->read = true;
      }
    
      printf("inserted into user's email. Now printing user's email inbox: \n");
      print_list(&(temp->email_list), print_email);
    }

  } else if (*type == 12) { //server received a DELETE EMAIL update from another server
    //Cast into Update type
    Update *update = (Update*) tmp_buf;
    //printf("we have received an update for a new email!\n");

    //TODO: updates_array needs to be taken into account; update our 2d array
    for (int i = 0; i < NUM_SERVERS; i++) {
      if (servers_in_partition[i]) {
        merge_matrix[i][update->timestamp.machine_index] = update->timestamp.message_index;
      }
    }

    /*
    printf("\nPrinting merge matrix: \n");
    for (int i = 0; i < NUM_SERVERS; i++) {
      for (int j = 0; j < NUM_SERVERS; j++) {
        printf("%d ", merge_matrix[i][j]);
      }
      printf("\n");
    }
    */
    //insert(&(array_of_updates_list[update->timestamp.machine_index]), (void*)update, compare_update);

    Update *existing_update = find(&(array_of_updates_list[update->timestamp.machine_index]),(void*)update, compare_update);
    if (existing_update != NULL) {
      //ignore update; not new!
      //printf("Update already exists! Should only show up in a merge!!\n");
      return;
    } else {      
      //update is new! We want to insert it into our array and process!!!!
      insert(&(array_of_updates_list[update->timestamp.machine_index]), (void*)update, compare_update);
    
      printf("Printing array of updates list: \n");
      for (int i = 0; i < NUM_SERVERS; i++) {
        printf("This is the linked list for index %d: \n", i);
        print_list(&(array_of_updates_list[i]), print_update);
      }
    
      //printf("this is subject: %s\n", update->email.emailInfo.subject);


      
      /*CODE NOT TESTED YET*/
      create_user_if_nonexistent(update->user_name);
      //ideally this will create the new user if it has not been made yet


      
      User *temp = (User*) find(&users_list, (void*)update->user_name, compare_users);
      assert(temp != NULL);
      //printf("User found! Here's their name: %s\n", temp->name);

      printf("timestamp of email (just counter and machine index): %d %d\n", update->timestamp_of_email.counter, update->timestamp_of_email.machine_index);

      Email *dummy = malloc(sizeof(Email));
      dummy->emailInfo.timestamp = update->timestamp_of_email;
    
      Email *email = find_backwards(&(temp->email_list), (void*)dummy, compare_email);
      free(dummy);

      if (email == NULL) {
        printf("\n\nNEW EMAIL IS BEING CREATED!!\n\n");
        Email *new_email = malloc(sizeof(Email));
        new_email->emailInfo.timestamp = update->timestamp_of_email;
        //new_email->emailInfo.timestamp.counter = update->timestamp_of_email.counter;
        //new_email->emailInfo.timestamp.machine_index = update->timestamp_of_email.machine_index
        new_email->read = false;
        new_email->exists = false;
        new_email->deleted = true;
            
        insert(&(temp->email_list),(void*) (new_email), compare_email);      
      } else {
        email->deleted = true;
      }
    
      printf("inserted into user's email. Now printing user's email inbox: \n");
      print_list(&(temp->email_list), print_email);

    }
    
  } else if (*type == 13) { //server received a NEW USER update from another server

    Update *update = (Update*) tmp_buf;
    //printf("we have received an update for a new user with name: %s\n", update->user_name);
    create_user_if_nonexistent(update->user_name);
    //print_list(&users_list, print_user);

    //now we want to process the array that we received and update our own 2d array with the info
    //memcpy(merge_matrix[update->timestamp.machine_index], update->updates_array, sizeof(update->updates_array));

    //consider if need to take max above or not; for now we say no
    // merge_matrix[my_machine_index][update->timestamp.machine_index] = update->timestamp.counter;

    //TODO:updates_array needs to be taken into account; update our 2d array
    for (int i = 0; i < NUM_SERVERS; i++) {
      if (servers_in_partition[i]) {
        merge_matrix[i][update->timestamp.machine_index] = update->timestamp.message_index;
      }
    }
    /*
    printf("\nPrinting merge matrix: \n");
    for (int i = 0; i < NUM_SERVERS; i++) {
      for (int j = 0; j < NUM_SERVERS; j++) {
        printf("%d ", merge_matrix[i][j]);
      }
      printf("\n");
      }*/

    //can't just insert it; want to check if we have the update already
    Update *existing_update = find(&(array_of_updates_list[update->timestamp.machine_index]),(void*)update, compare_update);
    if (existing_update != NULL) {
      //printf("Update already exists! Should only show up in a merge!!\n");
      return;
    } else {
      //update is new! We want to insert it into our array!!!!
      insert(&(array_of_updates_list[update->timestamp.machine_index]), (void*)update, compare_update);
      
      printf("Printing array of updates list: \n");
      for (int i = 0; i < NUM_SERVERS; i++) {
        printf("This is the linked list for index %d: \n", i);
        print_list(&(array_of_updates_list[i]), print_update);
      }
    }
  // **************************** parse reconciliation message **************************** //    
  } else if (*type == 20) { //server received a MERGEMATRIX from another server

    //why is this here?????

  } else { //unknown type!
    printf("Unknown type received! Exiting program...\n");
    exit(1);
  }
  
}



bool create_user_if_nonexistent(char name[MAX_NAME_LEN]) {
  User *temp = (User*) find(&users_list, &name[0], compare_users);

  if (temp == NULL) {
    //create new user
    User *user_to_insert = malloc(sizeof(User));
    strcpy(user_to_insert->name, name);
    //printf("before creating email list for user\n");
    create_list(&(user_to_insert->email_list), sizeof(Email));
    add_to_end(&users_list, user_to_insert);
    //printf("new user created!\n");
    return true;
  }

  return false;
}


/////////////////////////////////////////////////////////////////////////////////////////////
// *********** COMPARISON & PRINTING FUNCTION POINTERS FOR GENERIC LINKED LIST *********** //
/////////////////////////////////////////////////////////////////////////////////////////////


//Prints a user and their associated list of emails (For debugging)
void print_user(void *user) {
  User *temp = (User*) user;
  //printf("Username: %s", temp->name);
  print_list(&(temp->email_list), print_email);
}


//Returns if two users have the same name
int compare_users(void* user1, void* user2) {
  //printf("entered compare users\n");
  User *user_in_linked_list = (User*) user1;
  if (user_in_linked_list->name == NULL) {
    //printf("user is null\n");
  }

  char *user_search = (char*) user2;
  //printf("comparing %s and %s", (user_in_linked_list->name), user_search);
  //printf("with strcmp value: %d\n", strcmp(user_in_linked_list->name, user_search));

  return strcmp(user_in_linked_list->name, user_search); 
}


//Prints an email (For debug)
void print_email(void *email) {
  Email *temp_email = (Email*) email;
  printf("Email Timestamp- Counter: %d, machine_index: %d, message_index: %d\n",
        temp_email->emailInfo.timestamp.counter, temp_email->emailInfo.timestamp.machine_index, 
        temp_email->emailInfo.timestamp.message_index);
  printf("Email State- Deleted: %d Read: %d Exists: %d\n", temp_email->deleted, temp_email->read, temp_email->exists);

  printf("To: %s\nFrom: %s\nSubject: %s\n\n", 
        temp_email->emailInfo.to_field, temp_email->emailInfo.from_field, temp_email->emailInfo.subject);
}


//Compares emails first by counter, and then by machine index as tiebreaker
int compare_email(void* temp1, void* temp2) {
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
      printf("Two equal emails have been found!\n");
      return 0;
    }
  }
}


//Prints updates (for debugging)
void print_update(void* temp) {
  Update *update = (Update*) temp;
  printf("This is type: %d\nThis is message index: %d\n", update->type, update->timestamp.message_index);
}


//Compares updates by message index
int compare_update(void* update1, void* update2) {
  Update *one = (Update*) update1;
  Update *two = (Update*) update2;

  if (one->timestamp.message_index < two->timestamp.message_index) {
    return -1;
  } else if (one->timestamp.message_index > two->timestamp.message_index) {
    return 1;
  } else {
    printf("Error: this should not happen: have two updates with same message index from same machine\n");
    return 0;
  }
}


//Compares emails for the find method to get the nth email that exists and has not been deleted
int compare_email_for_find(void* temp1, void* temp2) {
  //find the nth from end exisiting and undeleted email
  int *the_one_we_want = (int*) temp2;
  Email *email_being_checked = (Email*) temp1;

  //printf("Email being checked: %s\n", email_being_checked->emailInfo.subject);
  //printf("This is the one we want %d\n", *the_one_we_want);
  //printf("This is num emails checked: %d\n", num_emails_checked);
  //global var called num_emails_checked
  if (email_being_checked->exists && !email_being_checked->deleted) {
    num_emails_checked++;
    if (*the_one_we_want == num_emails_checked) {
      return 0;
    }
  }
  return -1;
}


//Simple max retrieval function
int max(int first, int second) {
  return first >= second ? first : second;
}


//Simple min retrieval function
int min(int first, int second) {
  return first <= second ? first : second;
}


//Adds to header ONLY when the email exists and is not deleted
void add_to_struct_to_send(void *data) {
  Email *email = (Email*) data;
  if (email->exists && !email->deleted) {
    add_to_header(email);
  } 
}


//Adds an email to a header struct
void add_to_header(Email *email) {
  //Sends the header struct once it has been filled with 10 emails
  if (num_headers_added == 10) {
    //printf("Sending header!\n");
    SP_multicast(Mbox, AGREED_MESS, sender, 2, sizeof(HeaderForClient), (char*)client_header_response);
    
    num_headers_added = 0;
    free(client_header_response);
    client_header_response = NULL;
    client_header_response = malloc(sizeof(HeaderForClient));
    client_header_response->type = 1;
    client_header_response->done = false;
  }

  client_header_response->headers[num_headers_added].message_number = message_number_stamp;
  client_header_response->headers[num_headers_added].read = email->read;
  client_header_response->headers[num_headers_added].timestamp = email->emailInfo.timestamp;

  strcpy(client_header_response->headers[num_headers_added].sender, email->emailInfo.from_field);
  strcpy(client_header_response->headers[num_headers_added].subject, email->emailInfo.subject);
  message_number_stamp++;
  num_headers_added++;
}

//Send updates for a merge only if the message_index is strictly greater than min_seen_global
bool send_updates_for_merge(void* temp) {
  Update *might_be_sent = (Update*) temp;
  printf("^^^^^^^^^^^^^^^^^^^^^^\nHave entered send_update_for_merge\nThis is curr message index: %d\nThis is min update global: %d\nThis is updates sent so far: %d\nThis is what we want to send up to: %d\n",
         might_be_sent->timestamp.message_index, min_update_global[current_i], num_updates_sent_so_far[current_i], max_update_global[current_i]);
  if ((might_be_sent->timestamp.message_index > min_update_global[current_i]) && (num_updates_sent_so_far[current_i] <= 10)) {
    //TODO: need to set that the machine's index onto the update in a new field
    might_be_sent->index_of_machine_resending_update = my_machine_index;
    SP_multicast(Mbox, AGREED_MESS, group, 2, sizeof(Update), (char*)might_be_sent);
    num_updates_sent_so_far[current_i] = num_updates_sent_so_far[current_i] + 1; //im so tired...is this okay to do?
    min_update_global[current_i] = min_update_global[current_i] + 1; //we do this so that when we start again we aren't resending
    if (num_updates_sent_so_far[current_i] == 10 && (min_update_global[current_i] < max_update_global[current_i])) { //we've sent our max and need to keep sending
      return false; //this means we have more to send!!!!!
    }
    if ((min_update_global[current_i] == max_update_global[current_i])) {
      return true; //done sending
    }
  }
  return true; //done sending
}



bool can_delete_update(void* temp) {
  Update *update = (Update*) temp;
  //printf("entered can delete update!\nThis is message index on timestamp: %d\nThis is min_global: %d\n", update->timestamp.message_index, min_global);
  if (update->timestamp.message_index <= min_global) {
    return true; //this means we can delete since the index on the update is less than what everyone has
  }
  return false;
}


//Method to exit
static void Bye() {
  To_exit = 1;
  printf("\nBye.\n");
  SP_disconnect(Mbox);
  exit(0);
}



/*
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
*/
