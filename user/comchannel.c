#include "kernel/types.h"
#include "user/user.h"

int main() {
  int p[2];
  char msg[6];

  pipe(p);

  if (fork() == 0) {
    read(p[0], msg, sizeof(msg));
    printf("Got Message: %s\n", msg);
    exit(0);
  } else {
    write(p[1], "Hello", 6);
    wait(0);
  }

  exit(0);
}
