#include "kernel/types.h"

#include "user/user.h"



int add(int a, int b) {

    int result = 0;



    for(int i = 0; i < b; i++){

        result = result + a;

    }



    return result;

}



int main() {

    int x = 2;

    int y = 3;

    int z;



    z = add(x, y);



    printf("Result = %d\n", z);



    exit(0);

}
