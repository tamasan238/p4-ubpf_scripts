#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "使用方法: %s <子プロセス数>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int num_children = atoi(argv[1]);
    if (num_children <= 0) {
        fprintf(stderr, "子プロセス数は 1 以上でなければなりません\n");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < num_children; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork に失敗しました");
            return EXIT_FAILURE;
        } else if (pid == 0) {
            // 子プロセス
            execl("./child_program", "./child_program", NULL);

            // execl が失敗した場合
            perror("execl に失敗しました");
            exit(EXIT_FAILURE);
        }
        // 親プロセスは何もしないで次の子プロセスを生成
    }

    // 親プロセスは全ての子プロセスの終了を待機
    for (int i = 0; i < num_children; i++) {
        wait(NULL);
    }

    return EXIT_SUCCESS;
}
