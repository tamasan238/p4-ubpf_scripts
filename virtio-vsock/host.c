#include <sys/socket.h>
#include <linux/vm_sockets.h>
#include <unistd.h>
#include <stdio.h>

int main() {
    int s = socket(AF_VSOCK, SOCK_STREAM, 0);
    struct sockaddr_vm addr = {0};
    addr.svm_family = AF_VSOCK;
    addr.svm_port = 12345;
    addr.svm_cid = VMADDR_CID_ANY;

    bind(s, (struct sockaddr*)&addr, sizeof(addr));
    listen(s, 1);

    int client = accept(s, NULL, NULL);

    write(client, "Hello guest", 12);  // ゲストに返信
    char buf[128];
    int n = read(client, buf, sizeof(buf));  // ゲストから受信
    buf[n] = '\0';
    printf("Received from guest: %s\n", buf);

    close(client);
    close(s);
}

