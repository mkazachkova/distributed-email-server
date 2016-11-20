#include "generic_linked_list.h"

void printInt(void *n);
int compareInt(void* temp1, void* temp2);

int main() {
  List list;
  create_list(&list, sizeof(int));
  for (int i = 0; i < 20; i+=2) {
    add_to_end(&list, &i);
  }

  print_list(&list, printInt);
 

  //insert
  int my_int = 3;
  
  insert(&list,(void*)&my_int, compareInt);
  print_list(&list, printInt);

  int my_int2 = -1;

  insert(&list,(void*)&my_int2, compareInt);
  print_list(&list, printInt);

  int my_int3 = 21;

  insert(&list,(void*)&my_int3, compareInt);
  print_list(&list, printInt);

  List list2;
  create_list(&list2, sizeof(int));

  print_list(&list2, printInt);

  int my_int4 = 3;
  
  insert(&list2,(void*)&my_int4, compareInt);
  print_list(&list2, printInt);
  
  int my_int5 = -5;

  insert(&list2,(void*)&my_int5, compareInt);
  print_list(&list2, printInt);

  
  int my_int6 = 3;

  insert(&list2,(void*)&my_int6, compareInt);
  print_list(&list2, printInt);


  remove_from_end(&list2);
  print_list(&list2, printInt);

  remove_from_beginning(&list2);
  print_list(&list2, printInt);

  int my_int7 = 6;

  insert(&list2,(void*)&my_int7, compareInt);
  print_list(&list2, printInt);
  
  
  int my_int8 = 100;

  insert(&list2,(void*)&my_int8, compareInt);
  print_list(&list2, printInt);

  int my_int9 = -2;

  insert(&list2,(void*)&my_int9, compareInt);
  print_list(&list2, printInt);

  int my_int10 = 75;

  insert(&list2,(void*)&my_int10, compareInt);
  print_list(&list2, printInt);

  print_list_backwards(&list2, printInt);

  int find2 = 75;
  Node* temp2 = find(&list2,(void*)&find2, compareInt);
  int *printed = (int*)temp2->data;
  printf("%d\n", *printed);

  
  return 0;
}

void printInt(void *n) {
  printf(" %d", *(int *)n);
}

int compareInt(void* temp1, void* temp2) {
  int *one = (int*) temp1;
  int *two = (int*) temp2;

  return *one < *two ? -1 : *one == *two ? 0 : 1;
}
