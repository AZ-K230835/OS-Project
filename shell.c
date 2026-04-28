#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_INPUT 1024
#define MAX_ARGS 100
#define HISTORY_SIZE 100

char history[HISTORY_SIZE][MAX_INPUT];
int history_count = 0;

void handle_sigint(int sig) {
    printf("\nUse 'exit' to quit the shell.\n");
    printf("myShell> ");
    fflush(stdout);
}

void save_history(char input[]) {
    if (history_count < HISTORY_SIZE) {
        strcpy(history[history_count], input);
        history_count++;
    }
}

void show_history() {
    int i;
    for (i = 0; i < history_count; i++) {
        printf("%d %s\n", i + 1, history[i]);
    }
}

void parse_input(char input[], char *args[], int *background) {
    int i = 0;
    char *token;

    *background = 0;
    token = strtok(input, " ");

    while (token != NULL) {
        if (token[0] == '$') {
            char *env = getenv(token + 1);
            if (env != NULL) {
                args[i++] = env;
            }
        } else {
            args[i++] = token;
        }

        token = strtok(NULL, " ");
    }

    args[i] = NULL;

    if (i > 0 && strcmp(args[i - 1], "&") == 0) {
        *background = 1;
        args[i - 1] = NULL;
    }
}

int execute_builtin(char *args[]) {
    if (args[0] == NULL) {
        return 1;
    }

    if (strcmp(args[0], "exit") == 0) {
        exit(0);
    }

    if (strcmp(args[0], "pwd") == 0) {
        char cwd[MAX_INPUT];

        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("pwd failed");
        }

        return 1;
    }

    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            printf("No directory provided\n");
        } else {
            if (chdir(args[1]) != 0) {
                perror("cd failed");
            }
        }

        return 1;
    }

    if (strcmp(args[0], "history") == 0) {
        show_history();
        return 1;
    }

    return 0;
}

int main() {
    char input[MAX_INPUT];
    char input_copy[MAX_INPUT];
    char *args[MAX_ARGS];
    int background;

    signal(SIGINT, handle_sigint);

    while (1) {
        printf("myShell> ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        input[strcspn(input, "\n")] = '\0';

        if (strlen(input) == 0) {
            continue;
        }

        strcpy(input_copy, input);
        save_history(input_copy);

        parse_input(input, args, &background);

        if (execute_builtin(args)) {
            continue;
        }

        pid_t pid = fork();

        if (pid < 0) {
            perror("Fork failed");
        }

        else if (pid == 0) {
            if (execvp(args[0], args) < 0) {
                perror("Invalid command");
            }

            exit(1);
        }

        else {
            if (!background) {
                waitpid(pid, NULL, 0);
            } else {
                printf("[Running in background] PID: %d\n", pid);
            }
        }
    }

    return 0;
}