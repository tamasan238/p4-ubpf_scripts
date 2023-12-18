#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]){
    if(argc != 2) {
        printf("Usage: ./ubpf-test <ubpf byte code>\n");
        exit(1);
    }

    printf("%s\n", argv[1]);

    return 0;
}