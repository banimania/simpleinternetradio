#include "src/client.h"
#include <ncurses.h>

int main() {
  Client client = Client("127.0.0.1", 6666);
  return 0;
}
