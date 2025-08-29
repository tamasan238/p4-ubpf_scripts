// guest.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/vm_sockets.h>

#define CID_HOST 2
#define PORT     1234

int main() {
    // 共有メモリを開く
    int fd = open("/dev/shm/ivshmem", O_RDWR);
    if (fd < 0) { perror("open"); return 1; }
    char *shared = mmap(NULL, 1024, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (shared == MAP_FAILED) { perror("mmap"); return 1; }

    // データを書き込む
    strcpy(shared, "Hello from Guest!");

    // vsockでホストに通知
    int sock = socket(AF_VSOCK, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    struct sockaddr_vm addr = {
        .svm_family = AF_VSOCK,
        .svm_cid = CID_HOST,
        .svm_port = PORT,
    };

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        return 1;
    }

    char msg[] = "data-ready";
    send(sock, msg, strlen(msg), 0);

    close(sock);
    munmap(shared, 1024);
    close(fd);
    return 0;
}

