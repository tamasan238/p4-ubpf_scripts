#define _GNU_SOURCE
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <fcntl.h>
typedef struct
{
    unsigned char iv[16];
    unsigned char cipher_text[8];
    unsigned char auth_tag[16];
} Packet_count;

typedef struct
{
    long long ovs_thread_id;
    int p4runtime_id;
    Packet_count packet_count;
} Connection;

typedef struct {
    pid_t pid;
    bool in_use;
} P4runtime;

#define WAIT_TIME_US 1000

#define SHM_NAME "/dev/uio0"
#define SHM_SIZE (8 * 1024 * 1024)
#define VM_AREA 0
#define META_AREA (VM_AREA + 2 * 1024 * 1024)
#define PACKETS_AREA (META_AREA + 2 * 1024 * 1024)

#define SHM_SESSION_TABLE 0
#define SHM_TABLE_IS_LOCKED (SHM_SESSION_TABLE + sizeof(Connection) * MAX_CONNECTIONS)

#define MAX_CONNECTIONS 32
#define INIT_CLIENTS 1

#define UBPF_RUNTIME_PATH "/home/iwai/ubpf/build/bin/ubpf_test"
#define UBPF_RUNTIME "ubpf_test"
#define UBPF_BYTECODE "/home/iwai/p4c/testdata/p4_16_samples/ubpf.o"
#define TARGET_PROCESS "ubpf_test"

static int shm_fd = -1;
static void *shm_base = NULL;
static void *shm_meta = NULL;
static Connection *session_table = NULL;
static bool *table_locked = NULL;

static int server_sock = -1;
static int runtimes = 0;
static P4runtime runtimes_table[MAX_CONNECTIONS];

static void log_and_exit(const char *msg)
{
    syslog(LOG_ERR, "%s", msg);
    exit(EXIT_FAILURE);
}

static void safe_close(int fd)
{
    if (fd >= 0) {
        close(fd);
    }
}

static void shm_open_and_map(void)
{
    shm_fd = open(SHM_NAME, O_RDWR);
    if (shm_fd < 0) {
        log_and_exit("shm cannot open. please try with sudo");
    }

    shm_base = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 4096);
    if (shm_base == MAP_FAILED) {
        log_and_exit("shm cannot mmap. maybe not prepared ivshmem-server or ivshmem-uio drivers");
    }

    shm_meta = (char *)shm_base + META_AREA;
    session_table = (Connection *)((char *)shm_meta + SHM_SESSION_TABLE);
    table_locked = (bool *)((char *)shm_meta + SHM_TABLE_IS_LOCKED);
}

static void shm_initialize_table(void)
{
    for (int i = 0; i < MAX_CONNECTIONS; ++i) {
        session_table[i].ovs_thread_id = -1;
        session_table[i].p4runtime_id = -1;
    }
    *table_locked = false;
}

static void shm_unmap_and_close(void)
{
    if (shm_base && shm_base != MAP_FAILED) {
        munmap(shm_base, SHM_SIZE);
        shm_base = NULL;
    }
    safe_close(shm_fd);
    shm_fd = -1;
}

static void add_runtime_entry(pid_t pid)
{
    for (int i = 0; i < MAX_CONNECTIONS; ++i) {
        if (runtimes_table[i].pid == -1) {
            runtimes_table[i].pid = pid;
            runtimes_table[i].in_use = false;
            return;
        }
    }
}

static void init_runtimes_table(void)
{
    for (int i = 0; i < MAX_CONNECTIONS; ++i) {
        runtimes_table[i].pid = -1;
        runtimes_table[i].in_use = false;
    }
    runtimes = 0;
}

static void spawn_runtime_process(void)
{
    pid_t pid = fork();
    if (pid < 0) {
        syslog(LOG_ERR, "fork failure");
        return;
    }
    if (pid == 0) {
        execl(UBPF_RUNTIME_PATH, UBPF_RUNTIME, UBPF_BYTECODE, NULL);
        _exit(1);
    } else {
        add_runtime_entry(pid);
        runtimes++;
        syslog(LOG_INFO, "started child (PID=%d)", pid);
    }
}

static void start_children(int num)
{
    if (num > MAX_CONNECTIONS) num = MAX_CONNECTIONS;
    while (runtimes < num) {
        spawn_runtime_process();
    }
}

static void terminate_child(pid_t pid)
{
    if (pid > 0) {
        kill(pid, SIGTERM);
        waitpid(pid, NULL, 0);
    }
}

static void stop_all_children(void)
{
    for (int i = 0; i < MAX_CONNECTIONS; ++i) {
        if (runtimes_table[i].pid > 0) {
            terminate_child(runtimes_table[i].pid);
            runtimes_table[i].pid = -1;
            runtimes_table[i].in_use = false;
        }
    }
    runtimes = 0;
}

static void adjust_children(int target)
{
    if (target == runtimes) return;
    if (target > MAX_CONNECTIONS) target = MAX_CONNECTIONS;
    if (target > runtimes) {
        start_children(target);
    } else {
        for (int i = 0; i < MAX_CONNECTIONS && runtimes > target; ++i) {
            if (runtimes_table[i].pid > 0 && !runtimes_table[i].in_use) {
                terminate_child(runtimes_table[i].pid);
                runtimes_table[i].pid = -1;
                runtimes_table[i].in_use = false;
                runtimes--;
            }
        }
    }
}

static int find_free_runtime_index(void)
{
    for (int i = 0; i < MAX_CONNECTIONS; ++i) {
        if (runtimes_table[i].pid > 0 && !runtimes_table[i].in_use) {
            return i;
        }
    }
    return -1;
}

static void link_add(int idx)
{
    adjust_children(runtimes + 1);
    usleep(1000);
    int free_idx = find_free_runtime_index();
    if (free_idx >= 0) {
        runtimes_table[free_idx].in_use = true;
        session_table[idx].p4runtime_id = runtimes_table[free_idx].pid;
        syslog(LOG_WARNING, "Session created | OVS ID: %lld, P4 ID: %d, i: %d",
               session_table[idx].ovs_thread_id, session_table[idx].p4runtime_id, idx);
    } else {
        syslog(LOG_ERR, "no free runtime found after spawn");
    }
}

static void link_del(int idx)
{
    int pid = session_table[idx].p4runtime_id;
    for (int i = 0; i < MAX_CONNECTIONS; ++i) {
        if (runtimes_table[i].pid == pid) {
            runtimes_table[i].in_use = false;
            break;
        }
    }
    session_table[idx].p4runtime_id = -1;
    adjust_children(runtimes - 1);
}

static void check_ovs_sessions(void)
{
    for (int i = 0; i < MAX_CONNECTIONS; ++i) {
        if (session_table[i].ovs_thread_id > 0 && session_table[i].p4runtime_id < 0) {
            syslog(LOG_WARNING, "Found a new session | OVS ID: %lld, i: %d",
                   session_table[i].ovs_thread_id, i);
            link_add(i);
        }
        if (session_table[i].ovs_thread_id < 0 && session_table[i].p4runtime_id > 0) {
            syslog(LOG_WARNING, "Found a lost session | P4 ID: %d, i: %d",
                   session_table[i].p4runtime_id, i);
            link_del(i);
        }
    }
}

static void sigchld_handler(int signo)
{
    (void)signo;
    while (waitpid(-1, NULL, WNOHANG) > 0) {}
}

static void setup_signal_handlers(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigchld_handler;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGCHLD, &sa, NULL);
}

static void setup_server_socket(void)
{
    struct sockaddr_un addr;
    unlink(SOCKET_PATH);
    server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_sock < 0) {
        log_and_exit("socket failure");
    }
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
    if (bind(server_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        safe_close(server_sock);
        log_and_exit("bind failure");
    }
    if (listen(server_sock, 5) < 0) {
        safe_close(server_sock);
        log_and_exit("listen failure");
    }
    int flags = fcntl(server_sock, F_GETFL, 0);
    fcntl(server_sock, F_SETFL, flags | O_NONBLOCK);
}

static void handle_start_command(int client_fd)
{
    dprintf(client_fd, "すでに起動しています\n");
}

static void handle_update_command(int client_fd, const char *arg)
{
    int num = atoi(arg);
    adjust_children(num);
    dprintf(client_fd, "子プロセス数を %d 個に調整しました\n", num);
}

static void handle_stop_command(int client_fd)
{
    stop_all_children();
    dprintf(client_fd, "子プロセスをすべて停止しました\n");
}

static void handle_status_command(int client_fd)
{
    dprintf(client_fd, "子プロセス数: %d\n", runtimes);
    int reported = 0;
    for (int i = 0; i < MAX_CONNECTIONS && reported < runtimes; ++i) {
        if (runtimes_table[i].pid > 0) {
            dprintf(client_fd, "  PID: %d\n", runtimes_table[i].pid);
            reported++;
        }
    }
}

static void handle_shutdown_command(int client_fd)
{
    char shutdown_cmd[1024];
    snprintf(shutdown_cmd, sizeof(shutdown_cmd), "pkill %s", TARGET_PROCESS);
    dprintf(client_fd, "P4Launcherとすべての子プロセスを終了します\n");
    int ret = system(shutdown_cmd);
    dprintf(client_fd, "%d", ret);
}

static void handle_client(int client_fd)
{
    char buffer[BUFFER_SIZE];
    ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);
    if (n <= 0) {
        close(client_fd);
        return;
    }
    buffer[n] = '\0';

    syslog(LOG_INFO, "受信コマンド: %s", buffer);

    if (strncmp(buffer, "start ", 6) == 0) {
        handle_start_command(client_fd);
    } else if (strncmp(buffer, "update ", 7) == 0) {
        handle_update_command(client_fd, buffer + 7);
    } else if (strncmp(buffer, "stop", 4) == 0) {
        handle_stop_command(client_fd);
    } else if (strncmp(buffer, "status", 6) == 0) {
        handle_status_command(client_fd);
    } else if (strncmp(buffer, "shutdown", 8) == 0) {
        handle_shutdown_command(client_fd);
    } else {
        dprintf(client_fd, "不明なコマンド\n");
    }
    close(client_fd);
}

static void daemonize_process(void)
{
    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    if (setsid() < 0) exit(EXIT_FAILURE);

    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    if (chdir("/") < 0) exit(EXIT_FAILURE);
    umask(0);

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    int fd = open("/dev/null", O_RDWR);
    if (fd != STDIN_FILENO) exit(EXIT_FAILURE);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
}

int main(void)
{
    openlog("P4Launcher", LOG_PID | LOG_NDELAY, LOG_DAEMON);
    daemonize_process();
    setup_signal_handlers();
    shm_open_and_map();
    shm_initialize_table();
    init_runtimes_table();
    setup_server_socket();

    start_children(INIT_CLIENTS);

    while (1) {
        check_ovs_sessions();
        usleep(WAIT_TIME_US);

        int client_fd = accept(server_sock, NULL, NULL);
        if (client_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            } else {
                syslog(LOG_ERR, "accept failure");
                continue;
            }
        }
        handle_client(client_fd);
    }

    shm_unmap_and_close();
    closelog();
    return 0;
}


#ifdef OLD

#define _GNU_SOURCE
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>

typedef struct
{
    long long ovs_thread_id;
    int p4runtime_id;
} Connection;

typedef struct
{
    pid_t p4runtime_id;
    bool in_use;
} P4runtime;

#define WAIT_TIME 1
#define SHM_NAME "/dev/uio0"
#define SHM_SIZE (8 * 1024 * 1024) // 8MB

#define VM_AREA 0                                  // unused in this program
#define META_AREA (VM_AREA + 2 * 1024 * 1024)      // Start at 2MB
#define PACKETS_AREA (META_AREA + 2 * 1024 * 1024) // Start at 4MB unused in this program

#define SHM_SESSION_TABLE 0
#define SHM_TABLE_IS_LOCKED (SHM_SESSION_TABLE + sizeof(Connection) * MAX_CONNECTIONS)

// #define MAX_CONNECTIONS 512
// #define MAX_CONNECTIONS 8
#define MAX_CONNECTIONS 32
#define INIT_CLIENTS 1

#define UBPF_RUNTIME_PATH "/home/iwai/ubpf/build/bin/ubpf_test"
#define UBPF_RUNTIME "ubpf_test"
#define UBPF_BYTECODE "/home/iwai/p4c/testdata/p4_16_samples/ubpf.o"
#define TARGET_PROCESS "ubpf_test"

/* for debug */
// #define P4CTL "/home/iwai/p4-ubpf_scripts/p4/p4launcher/demo"
// #define P4RUNCOMMAND "/home/iwai/p4-ubpf_scripts/p4/p4launcher/demo"
// #define TARGET_PROCESS "demo"

void start_children(int num);
void stop_all_children(void);
void adjust_children(int num);

void daemonize(void);

void shm_start(void);
void shm_init(void);
void shm_end(void);

void check_ovs(void);
void link_add(int);
void link_del(int);

void handle_client(int client_sock);
void setup_signal_handlers(void);
void setup_server_socket(void);

// shm
int fd;
void *shm_ptr;
void *base_ptr;

// for shm
Connection *session;
bool *is_locked;

// fork
int server_sock;
int runtimes = 0; // can change in adjust_children()
P4runtime p4runtime[MAX_CONNECTIONS];

int main()
{
    openlog("P4Launcher", LOG_PID | LOG_NDELAY, LOG_DAEMON);
    daemonize();
    setup_signal_handlers();
    shm_start();
    shm_init();
    setup_server_socket();

    for (int i = 0; i < MAX_CONNECTIONS; i++)
    {
        p4runtime[i].p4runtime_id = -1;
        p4runtime[i].in_use = false;
    }

    start_children(INIT_CLIENTS);
    while (1)
    {
        check_ovs();
        usleep(WAIT_TIME);

        int client_fd = accept(server_sock, NULL, NULL);
        if (client_fd < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // usleep(5000000); // 5秒
                continue;
            }
            syslog(LOG_ERR, "accept failure");
            continue;
        }
        handle_client(client_fd);
    }

    shm_end();
    closelog();

    return 0;
}

void check_ovs(void)
{
    for (int i = 0; i < MAX_CONNECTIONS; i++)
    {
        // if(i<10){
        //     syslog(LOG_WARNING, "i: %d, OVS ID: %lld, P4 ID: %d",
        //             i, session[i].ovs_thread_id, session[i].p4runtime_id);
        // }
        if (session[i].ovs_thread_id > 0 && session[i].p4runtime_id < 0)
        {
            syslog(LOG_WARNING, "Found a new session | OVS ID: %lld, i: %d",
                session[i].ovs_thread_id, i);
            link_add(i);
        }
        if (session[i].ovs_thread_id < 0 && session[i].p4runtime_id > 0)
        {
            syslog(LOG_WARNING, "Found a lost session | P4 ID: %d, i: %d",
                session[i].p4runtime_id, i);
            link_del(i);
        }
    }
}

void link_add(int i)
{
    // start runtime
    adjust_children(runtimes + 1);
    usleep(1000); // wait for start
    for (int j = 0; j < MAX_CONNECTIONS; j++)
    {
        if (p4runtime[j].p4runtime_id != -1 && p4runtime[j].in_use == false)
        {
            // change status of p4runtime
            p4runtime[j].in_use = true;
            // add session
            session[i].p4runtime_id = p4runtime[j].p4runtime_id;
            syslog(LOG_WARNING, "Session created | OVS ID: %lld, P4 ID: %d, i: %d",
                session[i].ovs_thread_id, session[i].p4runtime_id, i);
            break;
        }
    }
}

void link_del(int i)
{
    // change status of p4runtime
    for (int j = 0; j < MAX_CONNECTIONS; j++)
    {
        if (p4runtime[j].p4runtime_id == session[i].p4runtime_id)
        {
            p4runtime[j].in_use = false;
            syslog(LOG_WARNING, "Session closed | OVS ID: %lld, P4 ID: %d, i: %d",
                session[i].ovs_thread_id, session[i].p4runtime_id, i);
            break;
        }
    }
    // delete session
    session[i].p4runtime_id = -1;
    // stop runtime
    adjust_children(runtimes - 1);
}

void shm_start(void)
{
    fd = open(SHM_NAME, O_RDWR);
    if (fd < 0)
    {
        syslog(LOG_ERR, "shm cannot open. please try with sudo");
        exit(EXIT_FAILURE);
    }
    syslog(LOG_INFO, "fd: %d，SHM_SIZE: %d", fd, SHM_SIZE);

    base_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 4096);
    if (base_ptr == MAP_FAILED)
    {
        syslog(LOG_ERR, "shm cannot mmap. maybe not prepared ivshmem-server or ivshmem-uio drivers");
        exit(EXIT_FAILURE);
    }
    syslog(LOG_INFO, "mapped to %p", base_ptr);

    shm_ptr = base_ptr + META_AREA;
}

void shm_init(void)
{
    session = (Connection *)(shm_ptr + SHM_SESSION_TABLE);
    for (int i = 0; i < MAX_CONNECTIONS; i++)
    {
        session[i].ovs_thread_id = -1;
        session[i].p4runtime_id = -1;
    }

    is_locked = (bool *)(shm_ptr + SHM_TABLE_IS_LOCKED);
    *is_locked = false;
}

void shm_end(void)
{
    munmap(base_ptr, SHM_SIZE);
    close(fd);
}

void start_children(int num)
{
    if (num > MAX_CONNECTIONS)
    {
        num = MAX_CONNECTIONS;
    }

    for (; runtimes < num; runtimes++)
    {
        pid_t pid = fork();
        if (pid < 0)
        {
            syslog(LOG_ERR, "fork failure");
            break;
        }
        else if (pid == 0)
        {
            // 子プロセス

            execl(UBPF_RUNTIME_PATH, UBPF_RUNTIME, UBPF_BYTECODE, NULL);
            // execl failure時
            syslog(LOG_ERR, "execl failure");
            _exit(1);
        }
        else
        {
            // 親プロセス

            // add p4runtime
            for (int i = 0; i < MAX_CONNECTIONS; i++)
            {
                if (p4runtime[i].p4runtime_id == -1)
                {
                    p4runtime[i].p4runtime_id = pid;
                    p4runtime[i].in_use = false;
                    break;
                }
            }
            syslog(LOG_INFO, "子プロセス %d 起動 (PID=%d)", runtimes, pid);
        }
    }
}

void stop_all_children()
{
    for (int i = 0; i < MAX_CONNECTIONS; i++)
    {
        if (p4runtime[i].p4runtime_id > 0)
        {
            kill(p4runtime[i].p4runtime_id, SIGTERM);
            syslog(LOG_INFO, "子プロセス %d 終了 (PID=%d)", i, p4runtime[i].p4runtime_id);
        }
    }

    // 終了待ち
    for (int i = 0; i < MAX_CONNECTIONS; i++)
    {
        if (p4runtime[i].p4runtime_id > 0)
        {
            int status;
            waitpid(p4runtime[i].p4runtime_id, &status, 0);
        }
    }

    runtimes = 0;
    for (int i = 0; i < MAX_CONNECTIONS; i++)
    {
        p4runtime[i].p4runtime_id = -1;
        p4runtime[i].in_use = false;
    }
}

void adjust_children(int target)
{
    if (target == runtimes)
    {
        return;
    }

    syslog(LOG_INFO, "【ランタイム数調整】現在数：%d, 目標値：%d", runtimes, target);

    if (target > MAX_CONNECTIONS)
    {
        syslog(LOG_INFO, "目標値をMAX_CONNECTIONS(%d)に変更", MAX_CONNECTIONS);
        target = MAX_CONNECTIONS;
    }

    if (target > runtimes)
    {
        syslog(LOG_INFO, "ランタイム追加");
        start_children(target);
    }
    else if (target < runtimes)
    {
        syslog(LOG_INFO, "ランタイム削減");
        for (int i = 0; i < MAX_CONNECTIONS; i++)
        {
            syslog(LOG_INFO, "i: %d, runtime_id: %d, in_use: %s", 
                i, p4runtime[i].p4runtime_id, 
                p4runtime[i].in_use ? "true" : "false");

            if (p4runtime[i].p4runtime_id > 0 && p4runtime[i].in_use == false)
            {
                if (kill(p4runtime[i].p4runtime_id, SIGTERM) == -1)
                {
                    syslog(LOG_ERR, "kill failure");
                }
                int status;
                if (waitpid(p4runtime[i].p4runtime_id, &status, 0) == -1)
                {
                    syslog(LOG_ERR, "waitpid failure");
                }
                syslog(LOG_INFO, "子プロセス %d 終了 (PID=%d)", i, p4runtime[i].p4runtime_id);
                p4runtime[i].p4runtime_id = -1;
            }
        }
        int count = 0;
        for (int i = 0; i < MAX_CONNECTIONS; i++)
        {
            if(p4runtime[i].in_use == true)
                count++;
        }
        runtimes = count;
        syslog(LOG_INFO, "ランタイム現在数: %d", runtimes);
    }
}

void sigchld_handler(int signo)
{
    (void)signo;
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}

void setup_signal_handlers()
{
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);
}

void setup_server_socket()
{
    struct sockaddr_un addr;

    unlink(SOCKET_PATH); // remove old sock file

    server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_sock < 0)
    {
        syslog(LOG_ERR, "socket failure");
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(server_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        syslog(LOG_ERR, "bind failure");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, 5) < 0)
    {
        syslog(LOG_ERR, "listen failure");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    int flags = fcntl(server_sock, F_GETFL, 0);
    fcntl(server_sock, F_SETFL, flags | O_NONBLOCK);
}

void handle_client(int client_fd)
{
    char buffer[BUFFER_SIZE];
    char shutdown_cmd[1024];
    snprintf(shutdown_cmd, sizeof(shutdown_cmd), "pkill %s", TARGET_PROCESS);

    ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);
    if (n <= 0)
    {
        syslog(LOG_ERR, "read failure");
        close(client_fd);
        return;
    }
    buffer[n] = '\0';

    syslog(LOG_INFO, "受信コマンド: %s", buffer);

    // コマンド解析例
    if (strncmp(buffer, "start ", 6) == 0)
    {
        // if (runtimes != 0) {
        //     dprintf(client_fd, "起動後はupdateコマンドを使用してください\n");
        //     close(client_fd);
        // }
        // int num = atoi(buffer + 6);
        // start_children(num);
        // dprintf(client_fd, "子プロセス数: %d\n", runtimes);
        // for (int i = 0; i < runtimes; i++) {
        //     dprintf(client_fd, "  PID: %d\n", p4runtime[i].p4runtime_id);
        // }
        dprintf(client_fd, "すでに起動しています\n");
    }
    else if (strncmp(buffer, "update ", 7) == 0)
    {
        int num = atoi(buffer + 7);
        adjust_children(num);
        dprintf(client_fd, "子プロセス数を %d 個に調整しました\n", num);
    }
    else if (strncmp(buffer, "stop", 4) == 0)
    {
        stop_all_children();
        dprintf(client_fd, "子プロセスをすべて停止しました\n");
    }
    else if (strncmp(buffer, "status", 6) == 0)
    {
        dprintf(client_fd, "子プロセス数: %d\n", runtimes);
        for (int i = 0; i < runtimes; i++)
        {
            dprintf(client_fd, "  PID: %d\n", p4runtime[i].p4runtime_id);
        }
    }
    else if (strncmp(buffer, "shutdown", 8) == 0)
    {
        dprintf(client_fd, "P4Launcherとすべての子プロセスを終了します\n");
        int ret = system(shutdown_cmd);
        dprintf(client_fd, "%d", ret);
    }
    else
    {
        dprintf(client_fd, "不明なコマンド\n");
    }
    close(client_fd);
}

void daemonize(void)
{
    pid_t pid;

    // first fork
    pid = fork();
    if (pid < 0)
    {
        syslog(LOG_ERR, "fork failure");
        exit(EXIT_FAILURE);
    }
    if (pid > 0)
    {
        // 親プロセスは終了
        exit(EXIT_SUCCESS);
    }

    // セッションリーダになる
    if (setsid() < 0)
    {
        syslog(LOG_ERR, "setsid failure");
        exit(EXIT_FAILURE);
    }

    // second fork (端末から完全に切り離す)
    pid = fork();
    if (pid < 0)
    {
        syslog(LOG_ERR, "fork failure (2)");
        exit(EXIT_FAILURE);
    }
    if (pid > 0)
    {
        exit(EXIT_SUCCESS);
    }

    // cd /
    if (chdir("/") < 0)
    {
        syslog(LOG_ERR, "chdir failure");
        exit(EXIT_FAILURE);
    }

    // reset filemode-mask
    umask(0);

    // redirect stdio for /dev/null
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    int fd = open("/dev/null", O_RDWR);
    if (fd != STDIN_FILENO)
    {
        syslog(LOG_ERR, "stdin open failure");
        exit(EXIT_FAILURE);
    }
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
}

#endif