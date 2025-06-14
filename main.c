// building my custom shell as codecrafters challenge (/project ?)

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>

int main() {
    setbuf(stdout, NULL);

    while (1) {
        char input[100];
        printf("$ ");
        if (!fgets(input, sizeof(input), stdin)) break;

        // Remove newline
        input[strcspn(input, "\n")] = '\0';

        // Tokenize into argv[]
        char *argv[20];
        int argc = 0;
        char *token = strtok(input, " ");
        while (token != NULL && argc < 19) {
            argv[argc++] = token;
            token = strtok(NULL, " ");
        }
        argv[argc] = NULL;

        if (argc == 0) continue; // skip empty input

        // Built-in: exit
        if (strcmp(argv[0], "exit") == 0 && (argc == 1 || (argc == 2 && strcmp(argv[1], "0") == 0))) {
            break;
        }

        // Built-in: echo
        else if (strcmp(argv[0], "echo") == 0) {
            for (int i = 1; i < argc; i++) {
                printf("%s", argv[i]);
                if (i < argc - 1) printf(" ");
            }
            printf("\n");
        }

        // built-in: pwd
        else if (strcmp(argv[0], "pwd") == 0) {
            char cwd[PATH_MAX];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("%s\n", cwd);
            } else {
                perror("pwd");
            }
        }

        // built-in: cd
       else if (strcmp(argv[0], "cd") == 0) {
            char* dir = argv[1];
            //char* dir_cpy = strdup(dir);
            if (chdir(dir) != 0){
                fprintf(stderr, "cd: %s: No such file or directory\n", dir);
            }
        }


        // Built-in: type
        else if (strcmp(argv[0], "type") == 0 && argc >= 2) {
            for (int i = 1; i < argc; i++) {
                char *cmd = argv[i];
                if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "echo") == 0 || strcmp(cmd, "type") == 0 || strcmp(cmd, "pwd") == 0 || strcmp(cmd, "cd") == 0) {
                    printf("%s is a shell builtin\n", cmd);
                } else {
                    char *path = getenv("PATH");
                    char *path_copy = strdup(path);
                    bool found = 0;
                    for (char *dir = strtok(path_copy, ":"); dir != NULL; dir = strtok(NULL, ":")) {
                        char fullpath[PATH_MAX];
                        snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, cmd);
                        if (access(fullpath, X_OK) == 0) {
                            printf("%s is %s\n", cmd, fullpath);
                            found = 1;
                            break;
                        }

                    }
                    free(path_copy);
                    if (!found) {
                        printf("%s: not found\n", cmd);
                    }
                }
            }
        }

        // External command
        else {
            pid_t pid = fork();
            if (pid == 0) {
                execvp(argv[0], argv);
                fprintf(stderr, "%s: command not found\n", argv[0]);
                exit(1); // child exits, parent continues loop
            } else if (pid > 0) {
                waitpid(pid, NULL, 0);
            } else {
                perror("fork");
            }
        }
    }

    return 0;
}