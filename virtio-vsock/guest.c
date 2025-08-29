#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <linux/vm_sockets.h>

#define SHM_SIZE 1024
#define CID_HOST 2       // ホストのvsock CIDは常に2
#define PORT     1234    // 任意のポート番号

int main() {
    // 共有メモリを開く (/dev/uio0)
    int fd = open("/dev/uio0", O_RDWR);
    if (fd < 0) { perror("open /dev/uio0"); return 1; }

    char *shared = mmap(NULL, SHM_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (shared == MAP_FAILED) { perror("mmap"); close(fd); return 1; }

    // データを書き込む
    strcpy(shared, "Hello from Guest!");

    // vsockで通知
    int sock = socket(AF_VSOCK, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    struct sockaddr_vm addr = {
        .svm_family = AF_VSOCK,
        .svm_cid    = CID_HOST,
        .svm_port   = PORT,
    };

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        return 1;
    }

    char msg[] = "data-ready";
    send(sock, msg, strlen(msg), 0);

    close(sock);
    munmap(shared, SHM_SIZE);
    close(fd);
    return 0;
}

