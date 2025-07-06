#include <stdio.h>
#include <unistd.h>

int main() {
    while (1) {
        printf("子プロセス PID=%d 動作中\n", getpid());
        sleep(5);
    }
}
