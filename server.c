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

static char         group[80] = "ssukard1mkazach1_2"; 
static char         server_own_group[80] = "ssukard1mkazach1_server_";    

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
int                 min_seen_global = -1;
int                 global_counter = 0;
int                 min_global = 0;
int                 num_updates_received = 0;

// Global variables used for sending header to client
int                 message_number_stamp = 1;
int                 num_headers_added;
HeaderForClient     *client_header_response;     

char                sender[MAX_GROUP_NAME];



//variables for flow control
int                 min_update_global[NUM_SERVERS] = { -1 };
int                 max_update_global[NUM_SERVERS] = { -1 };
int                 num_updates_sent_so_far[NUM_SERVERS] = { 0 };
bool                done_sending_for_merge[NUM_SERVERS] = { true };
int                 current_i = -1;
int                 num_updates_received_from_myself = 0;
int                 who_sends[NUM_SERVERS] = { -1 };
int                 num_sent_in_each_round = 0;


//Our own methods 
static void         Respond_To_Message();
static void         Bye();
static int          compare_users(void *user1, void *user2);
static int          compare_email(void *temp, void *temp2);
static int          compare_email_2(void *temp, void *temp2);
static int          compare_update(void* update1, void* update2);
static bool         create_user_if_nonexistent(char *name);
static bool         send_updates_for_merge(void *update);
static void         add_to_struct_to_send(void *data);
static void         add_to_header(Email *email);
static bool         can_delete_update(void* update);
static int          max(int one, int two);
static int          min(int one, int two);


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

  //Spread setup (copypasted from class_user sample code)
  sprintf(Server, "user_mk_ss");
  sprintf(Spread_name, "10050");

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

  
  // Putting whatever was received into tmp_buf, which will be cast into whatever type
  // it is by looking at the first digit!
  char *tmp_buf = malloc(MAX_PACKET_LEN);
  ret = SP_receive(Mbox, &service_type, sender, 100, &num_groups, target_groups,
                      &mess_type, &endian_mismatch, MAX_PACKET_LEN, (char*)tmp_buf);

  // **************************** PARSE MEMBERSHIP MESSAGES **************************** //

  //If transition message, do nothing
  if (Is_transition_mess(service_type)) {
    return;
  }

  //Parsing the server entering message (Should only be executed at the beginning of the program)
  if (Is_caused_join_mess(service_type)) {

    char first_char = sender[0];

    //If number received, then a NON-server group was joined by a client.
    if (first_char >= '0' && first_char <= '9') {
      return; //this means client has joined the group you've already joined. Do nothing
    }
    
    //Otherwise, it is a server group.

    //Change servers_in_partition array to reflect current servers in array
    for (int i = 0; i < num_groups; i++) {
      int num = atoi(&(target_groups[i][strlen(target_groups[i]) - 1]));
      servers_in_partition[num] = true;
      num_servers_in_partition++;
    }

    return;

  } else if (Is_caused_leave_mess(service_type) || Is_caused_disconnect_mess(service_type)) {
    //If server gets a leave message from a single client-server spread group, LEAVE THAT GROUP ALSO.
    char first_char = sender[0];

    //Received leave message from non-client server
    if (first_char >= '0' && first_char <= '9') {
      SP_leave(Mbox, sender);
      return; //this means client has left group between client and server; also leave client group
    }

    return;
  }


  //************************** RECONCILIATION PROCESS *************************//
  //This method is triggered whenever there is a change in membership detected!
  if (Is_caused_network_mess(service_type)) {     
    //    num_matrices_received = 0;

    int number_of_groups = num_groups;

    //If server gets a leave message from a single client-server spread group, LEAVE THAT GROUP ALSO.
    char first_char = sender[0];

    //If number received, then a NON-server group was partitioned
    if (first_char >= '0' && first_char <= '9') {
      SP_leave(Mbox, sender);
      return; 
    }
    
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


    //Allocate space for and populate MergeMatrix sent to other servers
    MergeMatrix *merge = malloc(sizeof(MergeMatrix));
    merge->type = 20;
    merge->machine_index = my_machine_index;

    //Copy local merge matrix into MergeMatrix to be sent
    for (int i = 0; i < NUM_SERVERS; i++) {
      memcpy(merge->matrix[i], merge_matrix[i], sizeof(merge_matrix[i]));
    }

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
      
      //If number received, then a NON-server group was partitioned
      if (first_char >= '0' && first_char <= '9') {
        continue; 
      }
      
      num_matrices_received++;
      
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
    int max_seen = 0;

    //resetting since now global
    for (int i = 0; i < NUM_SERVERS; i++) {
      who_sends[i] = -1;
    }


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
      knows a correct representation of what updates other processes have.
     */


    //reset.
    for (int i = 0; i < NUM_SERVERS; i++) {
      done_sending_for_merge[i] = true;
      min_update_global[i] = 0;
      max_update_global[i] = 0;
    }
    
    num_sent_in_each_round = 0;
    num_updates_received_from_myself = 0;
    //Do the actual sending of messages
    int min_seen = INT_MAX;
    for (int i = 0; i < NUM_SERVERS; i++) {
      min_seen = INT_MAX;

      //If who_sends[i] is our machine index, WE must send      
      if (who_sends[i] == my_machine_index) {        
        //Find the min for the process we are sending for and send from there until the end
        for (int j = 0; j < NUM_SERVERS; j++) {
          if (servers_in_partition[j]) {
            if (min_array[j][i] < min_seen) {
              min_seen = min_array[j][i];
            }
          }
        }
        min_update_global[i] = min_seen; //where we start from
        Update *last = get_tail(&(array_of_updates_list[i]));
        if (last == NULL) {
          done_sending_for_merge[i] = true;
          continue;
        }
        max_update_global[i] = last->timestamp.message_index; //where we end
        done_sending_for_merge[i] = false; //so that we know we are still sending
        current_i = i; //so that we know which index to use when using our NUMEROUS global arrays
        
        //Set the min_seen_global variable to the minimum we have seen 
        //(global only set here for safety)
        min_seen_global = min_seen;

        // now we start to send
        bool done  = forward_iterator(&(array_of_updates_list[i]), send_updates_for_merge);
        done_sending_for_merge[i] = done; 
      }

    }

    return;
  }


  // **************************** PARSE NON-MEMBERSHIP MESSAGES **************************** //
  //Cast first digit into integer to find out the type  
  int *type = (int*) tmp_buf;

  if (*type >= 10 && *type <= 13) {
    Update *temp = (Update*) tmp_buf;
    Update *existing_update = find(&(array_of_updates_list[temp->timestamp.machine_index]),(void*)temp, compare_update);
    if (existing_update == NULL) {
      num_updates_received++;                   
    }

    if (temp->index_of_machine_resending_update == my_machine_index) { //have received an update from ourselves that we resent
      num_updates_received_from_myself++;
    }
  }


   if (num_updates_received_from_myself == num_sent_in_each_round) {
    //time to keep on sending. boo
     num_sent_in_each_round = 0;
     num_updates_received_from_myself = 0;
     for(int i = 0; i < NUM_SERVERS; i++) {
       if(who_sends[i] == my_machine_index) { //one of the indexes i'm responsible for; keep sending
         if (!done_sending_for_merge[i]) { //means we have more to send for this machine
           current_i = i; //set our global variable
           num_updates_sent_so_far[i] = 0; //set this back to zero since it will be incrmemented in interator method
           bool done  = forward_iterator(&(array_of_updates_list[i]), send_updates_for_merge);
           done_sending_for_merge[i] = done;
         }
       }
     }
     
   }

   bool can_delete = true;
   for (int i = 0; i < NUM_SERVERS; i++) {
     if (!done_sending_for_merge[i]) {
       can_delete = false;
     }
   }
   
  
  if (num_updates_received >= NUM_FOR_DELETE_UPDATES && can_delete) {
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

  if (*type == 1) { // We received a "new user" message from the client when connection already established
    InfoForServer *info = (InfoForServer*) tmp_buf;

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


  } else if (*type == 2) { // We received a connection request with a potentially new user name attached to the request

    //We know that the message received was of type InfoForServer, so we can cast it accordingly
    InfoForServer *info = (InfoForServer*) tmp_buf;
    bool created_new_user = create_user_if_nonexistent(info->user_name);

    // Send an InfoForClient object back to client with a unique group name saying that the connection was successfully established
    InfoForClient *info_client = malloc(sizeof(InfoForClient));
    info_client->type = 4;
    char group_for_client[MAX_GROUP_NAME];

    //Generate group name from current time to guarantee uniqueness
    int seconds = time(NULL); //get current time
    sprintf(group_for_client, "%d", seconds);

    //Join group and send to client
    SP_join(Mbox, group_for_client); 
    strcpy(info_client->client_server_group_name, group_for_client);

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
    for (int i = num_headers_added; i < MAX_HEADERS_IN_PACKET; i++) {
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

    //Send the actual email body back to client
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
  // If *type is of type 10-13, we have RECEIVED A MESSAGE FROM A SERVER.

  } else if (*type == 10) { //server received a NEW EMAIL update from another server
    //Cast into Update type
    Update *update = (Update*) tmp_buf;
    create_user_if_nonexistent(update->email.emailInfo.to_field); //create new user if new user doesn't exist yet

    //update our own lamport counter
    lamport_counter = max(lamport_counter, update->email.emailInfo.timestamp.counter);

    // Update our 2d merge_matrix array
    for (int i = 0; i < NUM_SERVERS; i++) {
      if (servers_in_partition[i]) {
        merge_matrix[i][update->timestamp.machine_index] = update->timestamp.message_index;
      }
    }

    //this logic is used in the chance that we receive an update for an email that we already have; we ignore it!
    Update *existing_update = find(&(array_of_updates_list[update->timestamp.machine_index]),(void*)update, compare_update);

    User *user_found = find(&(users_list), update->email.emailInfo.to_field, compare_users);
    assert(user_found != NULL); //should never be null because we just created it if it's null




    Email *temp = find(&(user_found->email_list), (void*)&(update->email), compare_email_2);






    if (temp != NULL) {
      //email exists so don't want to process it
      return;
    }
    
    if (existing_update != NULL) {
      return;
    } else {
      //update is new! We want to insert it into our array!!!!
      insert(&(array_of_updates_list[update->timestamp.machine_index]), (void*)update, compare_update);
    
      User *temp = find(&users_list, (void*)update->email.emailInfo.to_field, compare_users);
      assert(temp != NULL);

      insert(&(temp->email_list),(void*) &(update->email), compare_email);
    }


  } else if (*type == 11) { //server received a READ EMAIL update from another server
    //Cast into Update type
    Update *update = (Update*) tmp_buf;

    //Update our 2d merge_matrix array
    for (int i = 0; i < NUM_SERVERS; i++) {
      if (servers_in_partition[i]) {
        merge_matrix[i][update->timestamp.machine_index] = update->timestamp.message_index;
      }
    }

    //insert(&(array_of_updates_list[update->timestamp.machine_index]), (void*)update, compare_update);

    Update *existing_update = find(&(array_of_updates_list[update->timestamp.machine_index]),(void*)update, compare_update);
    if (existing_update != NULL) {
      return;
    } else {
      //update is new! We want to insert it into our array!!!!
      insert(&(array_of_updates_list[update->timestamp.machine_index]), (void*)update, compare_update);

      create_user_if_nonexistent(update->user_name);
      //ideally this will create the new user if it has not been made yet
      
      User *temp = (User*) find(&users_list, (void*)update->user_name, compare_users);
      assert(temp != NULL);

      Email *dummy = malloc(sizeof(Email));
      dummy->emailInfo.timestamp = update->timestamp_of_email;
    
      Email *email = find_backwards(&(temp->email_list), (void*)dummy, compare_email);
      free(dummy);

      if (email == NULL) {
        Email *new_email = malloc(sizeof(Email));
        new_email->emailInfo.timestamp = update->timestamp_of_email;
        new_email->read = true;
        new_email->exists = false;
        new_email->deleted = false;
            
        insert(&(temp->email_list),(void*)(new_email), compare_email);      
      } else {
        email->read = true;
      }
    }


  } else if (*type == 12) { //server received a DELETE EMAIL update from another server
    //Cast into Update type
    Update *update = (Update*) tmp_buf;

    // Update our 2d merge_matrix array
    for (int i = 0; i < NUM_SERVERS; i++) {
      if (servers_in_partition[i]) {
        merge_matrix[i][update->timestamp.machine_index] = update->timestamp.message_index;
      }
    }

    Update *existing_update = find(&(array_of_updates_list[update->timestamp.machine_index]),(void*)update, compare_update);
    if (existing_update != NULL) {
      //ignore update; not new!
      return;

    } else {      
      //update is new! We want to insert it into our array and process!!!!
      insert(&(array_of_updates_list[update->timestamp.machine_index]), (void*)update, compare_update);
      
      create_user_if_nonexistent(update->user_name);
      //ideally this will create the new user if it has not been made yet

      
      User *temp = (User*) find(&users_list, (void*)update->user_name, compare_users);
      assert(temp != NULL);

      Email *dummy = malloc(sizeof(Email));
      dummy->emailInfo.timestamp = update->timestamp_of_email;
    
      Email *email = find_backwards(&(temp->email_list), (void*)dummy, compare_email);
      free(dummy);

      if (email == NULL) {
        Email *new_email = malloc(sizeof(Email));
        new_email->emailInfo.timestamp = update->timestamp_of_email;

        new_email->read = false;
        new_email->exists = false;
        new_email->deleted = true;
            
        insert(&(temp->email_list),(void*) (new_email), compare_email);      
      } else {
        email->deleted = true;
      }
    }
    

  } else if (*type == 13) { //server received a NEW USER update from another server

    Update *update = (Update*) tmp_buf;
    create_user_if_nonexistent(update->user_name);

    // update our 2d merge_matrix array
    for (int i = 0; i < NUM_SERVERS; i++) {
      if (servers_in_partition[i]) {
        merge_matrix[i][update->timestamp.machine_index] = update->timestamp.message_index;
      }
    }

    //can't just insert it; want to check if we have the update already
    Update *existing_update = find(&(array_of_updates_list[update->timestamp.machine_index]),(void*)update, compare_update);
    if (existing_update != NULL) {
      return;
    } else {
      //update is new! We want to insert it into our array!!!!
      insert(&(array_of_updates_list[update->timestamp.machine_index]), (void*)update, compare_update);
    }
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
    create_list(&(user_to_insert->email_list), sizeof(Email));
    add_to_end(&users_list, user_to_insert);
    return true;
  }

  return false;
}


/////////////////////////////////////////////////////////////////////////////////////////////
// *********** COMPARISON & PRINTING FUNCTION POINTERS FOR GENERIC LINKED LIST *********** //
/////////////////////////////////////////////////////////////////////////////////////////////


//Returns if two users have the same name
int compare_users(void* user1, void* user2) {
  User *user_in_linked_list = (User*) user1;
  char *user_search = (char*) user2;

  return strcmp(user_in_linked_list->name, user_search); 
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
      return 0;
    }
  }
}




//Compares emails first by counter, and then by machine index as tiebreaker
int compare_email_2(void* temp1, void* temp2) {
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
      if (!(one->exists)) {
        one->emailInfo = two->emailInfo;
        one->exists = true; //will be shown to user
      }
      return 0;
    }
  }
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
    return 0;
  }
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
  //Sends the header struct once it has been filled with MAX_HEADERS_IN_PACKET emails
  if (num_headers_added == MAX_HEADERS_IN_PACKET) {
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

  if ((might_be_sent->timestamp.message_index > min_update_global[current_i]) && (num_updates_sent_so_far[current_i] <= 10)) {
    num_sent_in_each_round++;
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
