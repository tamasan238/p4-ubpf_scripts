#define _GNU_SOURCE
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>

static pid_t child_pids[MAX_CHILDREN];
static int child_count = 0;

void run_server_loop(int server_fd);
void start_children(int num);
void stop_all_children(void);
void adjust_children(int num);
void handle_client(int client_sock);
void setup_signal_handlers(void);
int setup_server_socket(void);
void daemonize(void);
void cleanup(void);

int main() {
    openlog("P4Launcher", LOG_PID | LOG_NDELAY, LOG_DAEMON);
    daemonize();
    setup_signal_handlers();
    int server_sock = setup_server_socket();
    run_server_loop(server_sock);
    cleanup();
    closelog();

    return 0;
}

void run_server_loop(int server_fd) {
    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            syslog(LOG_ERR, "accept 失敗");
            continue;
        }
        handle_client(client_fd);
    }
}


void start_children(int num) {
    if (num > MAX_CHILDREN) {
        num = MAX_CHILDREN;
    }

    syslog(LOG_INFO,"a");

    for (; child_count < num; child_count++) {
        pid_t pid = fork();
        if (pid < 0) {
            syslog(LOG_ERR, "fork 失敗");
            break;
        } else if (pid == 0) {
            // 子プロセス： child_program を実行
            execl(P4CTL, P4RUNCOMMAND, NULL);
            // execl 失敗時
            syslog(LOG_ERR, "execl 失敗");
            _exit(1);
        } else {
            // 親プロセス： PID を保持
            child_pids[child_count] = pid;
            syslog(LOG_INFO,"子プロセス %d 起動 (PID=%d)\n", child_count, pid);
        }
    }
}

void stop_all_children() {
    for (int i = 0; i < child_count; i++) {
        if (child_pids[i] > 0) {
            if (kill(child_pids[i], SIGTERM) == -1) {
                // syslog(LOG_ERR, "kill 失敗");
            }
            syslog(LOG_INFO,"子プロセス %d 終了 (PID=%d)\n", i, child_pids[i]);
        }
    }

    // 終了待ち
    for (int i = 0; i < child_count; i++) {
        if (child_pids[i] > 0) {
            int status;
            if (waitpid(child_pids[i], &status, 0) == -1) {
                // syslog(LOG_ERR, "waitpid 失敗");
            }
        }
    }

    child_count = 0;
    memset(child_pids, 0, sizeof(child_pids));
}

void adjust_children(int target) {
    if (target > MAX_CHILDREN) {
        target = MAX_CHILDREN;
    }

    if (target > child_count) {
        start_children(target);
    } else if (target < child_count) {
        for (int i = child_count - 1; i >= target; i--) {
            if (child_pids[i] > 0) {
                if (kill(child_pids[i], SIGTERM) == -1) {
                    syslog(LOG_ERR, "kill 失敗");
                }
                int status;
                if (waitpid(child_pids[i], &status, 0) == -1) {
                    syslog(LOG_ERR, "waitpid 失敗");
                }
                syslog(LOG_INFO,"子プロセス %d 終了 (PID=%d)\n", i, child_pids[i]);
                child_pids[i] = 0;
            }
            child_count--;
        }
    }
}

void sigchld_handler(int signo) {
    (void)signo;
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

void setup_signal_handlers() {
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);
}

int setup_server_socket() {
    int server_fd;
    struct sockaddr_un addr;

    unlink(SOCKET_PATH);  // 古いソケットファイルを削除

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        syslog(LOG_ERR, "socket 失敗");
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        syslog(LOG_ERR, "bind 失敗");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0) {
        syslog(LOG_ERR, "listen 失敗");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    return server_fd;
}

void handle_client(int client_fd) {
    char buffer[BUFFER_SIZE];
    char shutdown_cmd[1024];
    snprintf(shutdown_cmd, sizeof(shutdown_cmd), "pkill %s", TARGET_PROCESS);

    ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);
    if (n <= 0) {
        syslog(LOG_ERR, "read 失敗");
        close(client_fd);
        return;
    }
    buffer[n] = '\0';

    syslog(LOG_INFO,"受信コマンド: %s\n", buffer);

    // コマンド解析例
    if (strncmp(buffer, "start ", 6) == 0) {
        if (child_count != 0) {
            dprintf(client_fd, "起動後はupdateコマンドを使用してください\n");
            close(client_fd);
        }
        int num = atoi(buffer + 6);
        start_children(num);
        dprintf(client_fd, "子プロセス数: %d\n", child_count);
        for (int i = 0; i < child_count; i++) {
            dprintf(client_fd, "  PID: %d\n", child_pids[i]);
        }
    } else if (strncmp(buffer, "update ", 7) == 0) {
        int num = atoi(buffer + 7);
        adjust_children(num);
        dprintf(client_fd, "子プロセス数を %d 個に調整しました\n", num);
    } else if (strncmp(buffer, "stop", 4) == 0) {
        stop_all_children();
        dprintf(client_fd, "子プロセスをすべて停止しました\n");
    } else if (strncmp(buffer, "status", 6) == 0) {
        dprintf(client_fd, "子プロセス数: %d\n", child_count);
        for (int i = 0; i < child_count; i++) {
            dprintf(client_fd, "  PID: %d\n", child_pids[i]);
        }
    } else if (strncmp(buffer, "shutdown", 8) == 0) {
        dprintf(client_fd, "P4Launcherとすべての子プロセスを終了します\n");
        system(shutdown_cmd);
    } else {
        dprintf(client_fd, "不明なコマンド\n");
    }
    close(client_fd);
}

void daemonize(void) {
    pid_t pid;

    // 1回目の fork
    pid = fork();
    if (pid < 0) {
        syslog(LOG_ERR, "fork 失敗");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        // 親プロセスは終了
        exit(EXIT_SUCCESS);
    }

    // セッションリーダになる
    if (setsid() < 0) {
        syslog(LOG_ERR, "setsid 失敗");
        exit(EXIT_FAILURE);
    }

    // 2回目の fork (端末から完全に切り離す)
    pid = fork();
    if (pid < 0) {
        syslog(LOG_ERR, "fork 失敗 (2回目)");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    // ワーキングディレクトリをルートに変更
    if (chdir("/") < 0) {
        syslog(LOG_ERR, "chdir 失敗");
        exit(EXIT_FAILURE);
    }

    // ファイルモードマスクをリセット
    umask(0);

    // 標準入出力を /dev/null にリダイレクト
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    int fd = open("/dev/null", O_RDWR);
    if (fd != STDIN_FILENO) {
        syslog(LOG_ERR, "stdin open 失敗");
        exit(EXIT_FAILURE);
    }
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
}