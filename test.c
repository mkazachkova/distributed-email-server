#include "generic_linked_list.h"

void printInt(void *n);

int main() {
  List list;
  create_list(&list, sizeof(int));
  for (int i = 0; i < 10; i++) {
    add_to_end(&list, &i);
  }

  print_list(&list, printInt);
  printf("\n");

  remove_from_beginning(&list);
  print_list(&list, printInt);
  printf("\n");

  remove_from_end(&list);
  print_list(&list, printInt);
  printf("\n");


  remove_from_beginning(&list);
  print_list(&list, printInt);
  printf("\n");

  remove_from_beginning(&list);
  print_list(&list, printInt);
  printf("\n");

  remove_from_beginning(&list);
  print_list(&list, printInt);
  printf("\n");

  remove_from_beginning(&list);
  print_list(&list, printInt);
  printf("\n");

  remove_from_end(&list);
  print_list(&list, printInt);
  printf("\n");


  remove_from_end(&list);
  print_list(&list, printInt);
  printf("\n");


  remove_from_end(&list);
  print_list(&list, printInt);
  printf("\n");

  printf("before\n");
  remove_from_end(&list);
  printf("after remove\n");
  print_list(&list, printInt);
  printf("\n");

  printf("before\n");
  remove_from_end(&list);
  print_list(&list, printInt);
  printf("\n");

  printf("end");
  remove_from_end(&list);
  print_list(&list, printInt);
  printf("\n");

  remove_from_end(&list);
  print_list(&list, printInt);
  printf("\n");

  

  
  return 0;
}

void printInt(void *n) {
  printf(" %d", *(int *)n);
}
