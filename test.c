#include "generic_linked_list.h"

int main() {
  List list;
  create_list(&list, sizeof(int));
  for (int i = 0; i < 10; i++) {
    add_to_end(&list, &i);
  }
  
  return 0;
}
