#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(2, "Usage: myfirstprogram <ticks>\n");
    exit(1);
  }

  int total_ticks = atoi(argv[1]);

  for (int i = 1; i <= total_ticks; i++) {
    pause(1);
    printf("tick %d\n", i);
  }

  exit(0);
}
