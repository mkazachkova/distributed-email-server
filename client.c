/*
Sarah Sukardi
Mariya Kazachkova
Distributed Systems: Final Project
December 9, 2016
*/

#include "sp.h"
#include "net_include.h"

#define int32u unsigned int

static char         User[80];
static char         Spread_name[80];

static char         Private_group[MAX_GROUP_NAME];
static mailbox      Mbox;
static int          Num_sent;
static unsigned int Previous_len;

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

//Variables that I (Sarah) added
static int                curr_server;
static char[MAX_NAME_LEN] curr_user;
static bool               logged_in = false;
static bool               connected_to_server = false; 



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
  char  command[130];
  char  mess[MAX_MESSLEN];
  char  group[80];
  char  groups[10][MAX_GROUP_NAME];
  int   num_groups;
  unsigned int  mess_len;
  int ret;
  int i;

  for(i = 0; i < sizeof(command); i++) {
    command[i] = 0;
  }

  if(fgets(command, 130, stdin) == NULL) {
    Bye();  
  }

  switch(command[0]) {
    //Login with a username
    case 'u':
      ret = sscanf( &command[2], "%s", group );
      if (ret < 1) {
        printf(" invalid group \n");
        break;
      }
      ret = SP_join( Mbox, group );
      if (ret < 0) SP_error( ret );
      break;

    //Connect to a specific mail server
    case 'c':
      ret = sscanf( &command[2], "%s", group );
      if (ret < 1) {
        printf(" invalid group \n");
        break;
      }
      ret = SP_leave( Mbox, group );
      if (ret < 0) SP_error( ret );
      break;

    //List the headers of received mail
    case 'l':
      num_groups = sscanf(&command[2], "%s%s%s%s%s%s%s%s%s%s", 
            groups[0], groups[1], groups[2], groups[3], groups[4],
            groups[5], groups[6], groups[7], groups[8], groups[9] );
      if (num_groups < 1) {
        printf(" invalid group \n");
        break;
      }
      printf("enter message: ");
      if (fgets(mess, 200, stdin) == NULL) {
        Bye();
      }

      mess_len = strlen(mess);
      ret= SP_multigroup_multicast( Mbox, SAFE_MESS, num_groups, (const char (*)[MAX_GROUP_NAME]) groups, 1, mess_len, mess );
      if (ret < 0) {
        SP_error(ret);
        Bye();
      }

      Num_sent++;
      break;

    //Mail a message to a user
    case 'm':
      num_groups = sscanf(&command[2], "%s%s%s%s%s%s%s%s%s%s", 
            groups[0], groups[1], groups[2], groups[3], groups[4],
            groups[5], groups[6], groups[7], groups[8], groups[9] );

      if(num_groups < 1) {
        printf(" invalid group \n");
        break;
      }

      printf("nter message: ");
      mess_len = 0;
      while (mess_len < MAX_MESSLEN) {
        if (fgets(&mess[mess_len], 200, stdin) == NULL) {
          Bye();
        }

        if (mess[mess_len] == '\n') {
          break;
        }
        
        mess_len += strlen(&mess[mess_len]);
      }
      ret = SP_multigroup_multicast( Mbox, SAFE_MESS, num_groups, (const char (*)[MAX_GROUP_NAME]) groups, 1, mess_len, mess );
      if (ret < 0) {
        SP_error(ret);
        Bye();
      }

      Num_sent++;
      break;

    //Delete a mail message
    case 'd':
      ret = sscanf( &command[2], "%s", group );
      if (ret != 1) {
        strcpy( group, "dummy_group_name" );
      }
      printf("enter size of each message: ");
      if (fgets(mess, 200, stdin) == NULL) {
        Bye();
      }

      ret = sscanf(mess, "%u", &mess_len );
      if (ret != 1) {
        mess_len = Previous_len;
      }
      if (mess_len > MAX_MESSLEN) {
        mess_len = MAX_MESSLEN;
      }

      Previous_len = mess_len;
      printf("sending 10 messages of %u bytes\n", mess_len );
      for (i = 0; i < 10; i++) {
        Num_sent++;
        sprintf( mess, "mess num %d ", Num_sent );
        ret= SP_multicast( Mbox, FIFO_MESS, group, 2, mess_len, mess );

        if (ret < 0) {
          SP_error( ret );
          Bye();
        }
        printf("sent message %d (total %d)\n", i+1, Num_sent );
      }
      break;

    //Read a received mail message
    case 'r':
      Read_message();
      break;

    //Print the membership of the mail servers in the current mail server's network component
    case 'p':
      ret = SP_poll( Mbox );
      printf("Polling sais: %d\n", ret );
      break;

    //Exit the program
    case 'q':
      Bye();
      break;


    default:
      printf("\nUnknown commnad\n");
      Print_menu();
      break;
  }

  printf("\nUser> ");
  fflush(stdout);

}

static void Read_message() {

  static  char     mess[MAX_MESSLEN];
          char     sender[MAX_GROUP_NAME];
          char     target_groups[MAX_MEMBERS][MAX_GROUP_NAME];
          membership_info  memb_info;
          vs_set_info      vssets[MAX_VSSETS];
          unsigned int     my_vsset_index;
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

//Takes in command-line args for spread name and user
static void Usage(int argc, char *argv[]) {
  sprintf(User, "user");
  sprintf(Spread_name, "4803");

  while (--argc > 0) {
    argv++;

    if(!strncmp(*argv, "-u", 2)) { //if the next 2 characters are equal to -u (is what it means)
      if (argc < 2) { Print_help(); } //apparently invalid amt. of args
      strcpy(User, argv[1]);
      argc--; argv++;
    } else if (!strncmp(*argv, "-r", 2)) {
      strcpy(User, "");
    } else if (!strncmp(*argv, "-s", 2)) {
      if (argc < 2) { Print_help(); }
      strcpy(Spread_name, argv[1]); 
      argc--; argv++;
    } else {
      Print_help();
    }
  }
}

static void Print_help() {
  printf( "Usage: spuser\n%s\n%s\n%s\n",
    "\t[-u <user name>]  : unique (in this machine) user name",
    "\t[-s <address>]    : either port or port@machine",
    "\t[-r ]             : use random user name");
  exit( 0 );
}

static void Bye() {
  To_exit = 1;
  printf("\nBye.\n");
  SP_disconnect( Mbox );
  exit( 0 );
}
