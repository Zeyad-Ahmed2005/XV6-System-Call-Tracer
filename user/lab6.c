#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main() {
    int p[2];
    char *msg = "This is the Child message";
    char buf[64];

    if (pipe(p) < 0) {
        printf("pipe failed\n");
        exit(1);
    }

    int pid = fork();

    if (pid < 0) {
        printf("fork failed\n");
        exit(1);
    }

    if (pid == 0) {
        close(p[0]);
        // Hardcode length 17 for "hello from child" to avoid strlen
        write(p[1], msg, 17); 
        close(p[1]);
        exit(0);
    } 
    else {
        close(p[1]);
        // read returns the number of bytes actually read
        int n = read(p[0], buf, sizeof(buf));
        if(n > 0) {
            write(1, buf, n);
            write(1, "\n", 1);
        }
        close(p[0]);
        wait(0);
        exit(0);
    }
}
