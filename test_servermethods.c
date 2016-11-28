/*
Sarah Sukardi
Mariya Kazachkova
Distributed Systems: Final Project
December 9, 2016
*/

#include "net_include.h"

#define int32u unsigned int

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
