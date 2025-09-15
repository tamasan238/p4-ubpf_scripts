#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>

#define HOST "olive-vm1.ksl.ci.kyutech.ac.jp"
#define PORT "80"

int main() {
    while (1) {
        struct addrinfo hints, *res;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;       // IPv4
        hints.ai_socktype = SOCK_STREAM; // TCP

        int status = getaddrinfo(HOST, PORT, &hints, &res);
        if (status != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
            sleep(1);
            continue;
        }

        int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sock < 0) {
            perror("ソケット作成失敗");
            freeaddrinfo(res);
            sleep(1);
            continue;
        }

        if (connect(sock, res->ai_addr, res->ai_addrlen) == 0) {
            printf("接続成功: %s:%s\n", HOST, PORT);
        } else {
            printf("接続失敗: %s\n", strerror(errno));
        }

        close(sock);
        freeaddrinfo(res);
        usleep(0.5*1000*1000);  // 0.5s
    }

    return 0;
}
