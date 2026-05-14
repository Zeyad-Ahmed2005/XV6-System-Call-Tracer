#include "kernel/types.h"
#include "user/user.h"

int main() {
    int pid;
    pid = fork();
    printf("fork() returned %d\n", pid);

    if(pid == 0){
        printf("child\n");
        char *args[] = {"ls", 0};
        exec("ls", args);
    } 
    else {
        printf("parent\n");
        wait(0);
    }
	printf("Finished\n");
	exit(0);
}
