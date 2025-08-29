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
    char buf[128];
    read(client, buf, sizeof(buf));
    printf("Received: %s\n", buf);
    close(client);
    close(s);
}

