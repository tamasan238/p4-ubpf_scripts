#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "使用法: %s <コマンド>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket 失敗");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect 失敗");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // コマンド送信
    char command[BUFFER_SIZE];
    snprintf(command, sizeof(command), "%s", argv[1]);
    for (int i = 2; i < argc; i++) {
        strncat(command, " ", sizeof(command) - strlen(command) - 1);
        strncat(command, argv[i], sizeof(command) - strlen(command) - 1);
    }

    if (strlen(command) >= BUFFER_SIZE - 1) {
        fprintf(stderr, "コマンドが長すぎます\n");
        close(sock);
        exit(EXIT_FAILURE);
    }

    if (write(sock, command, strlen(command)) < 0) {
        perror("write 失敗");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // 応答表示
    char buf[BUFFER_SIZE];
    read(sock, buf, sizeof(buf));

    close(sock);
    return 0;
}