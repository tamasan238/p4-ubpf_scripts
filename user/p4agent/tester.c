#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/select.h>

#define HOST "olive-vm1.ksl.ci.kyutech.ac.jp"
#define PORT "80"
#define TIMEOUT_SEC 2
#define TIMEOUT_USEC 0

int main() {
    while (1) {
        struct addrinfo hints, *res;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

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

        // ノンブロッキングに設定
        int flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);

        int ret = connect(sock, res->ai_addr, res->ai_addrlen);
        if (ret < 0 && errno != EINPROGRESS) {
            printf("接続失敗: %s\n", strerror(errno));
            close(sock);
            freeaddrinfo(res);
            usleep(500000);
            continue;
        }

        // select でタイムアウト待ち
        fd_set wfds;
        FD_ZERO(&wfds);
        FD_SET(sock, &wfds);
        struct timeval tv;
        tv.tv_sec = TIMEOUT_SEC;
        tv.tv_usec = TIMEOUT_USEC;

        ret = select(sock + 1, NULL, &wfds, NULL, &tv);
        if (ret > 0) {
            int err;
            socklen_t len = sizeof(err);
            getsockopt(sock, SOL_SOCKET, SO_ERROR, &err, &len);
            if (err == 0) {
                printf("接続成功: %s:%s\n", HOST, PORT);
            } else {
                printf("接続失敗: %s\n", strerror(err));
            }
        } else if (ret == 0) {
            printf("タイムアウト\n");
        } else {
            perror("select失敗");
        }

        close(sock);
        freeaddrinfo(res);
        usleep(0.5 * 1000 * 1000);  // 0.5 sec
    }

    return 0;
}
