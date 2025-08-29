#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <linux/vm_sockets.h>

#define SHM_SIZE 1024
#define PORT     1234

int main() {
    // vsockサーバを立てる
    int sock = socket(AF_VSOCK, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    struct sockaddr_vm addr = {
        .svm_family = AF_VSOCK,
        .svm_cid    = VMADDR_CID_ANY,
        .svm_port   = PORT,
    };

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }
    listen(sock, 1);

    int client = accept(sock, NULL, NULL);
    if (client < 0) { perror("accept"); return 1; }

    char buf[128] = {0};
    recv(client, buf, sizeof(buf), 0);
    printf("Notification received: %s\n", buf);

    // 共有メモリを開く (ホスト側ではQEMU起動時に指定した mem-path)
    int fd = open("/dev/shm/ivshmem", O_RDONLY);
    if (fd < 0) { perror("open /dev/shm/ivshmem"); return 1; }

    char *shared = mmap(NULL, SHM_SIZE, PROT_READ, MAP_SHARED, fd, 0);
    if (shared == MAP_FAILED) { perror("mmap"); return 1; }

    printf("Shared memory content: %s\n", shared);

    munmap(shared, SHM_SIZE);
    close(fd);
    close(client);
    close(sock);
    return 0;
}

