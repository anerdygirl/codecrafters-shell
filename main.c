// building my custom shell as codecrafters challenge (/project ?)

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>    //PATH_MAX
#include <unistd.h>    //handles paths and stuff. also uid for ~
#include <sys/types.h> //processes?
#include <sys/wait.h>  //same here
#include <stdbool.h>
#include <pwd.h>   //home directory
#include <fcntl.h> //redirect output

void remove_char(char *str, int pos)
{
    int len = strlen(str);
    if (pos < 0 || pos >= len)
        return; // out of bounds
    for (int i = pos; i < len; i++)
        str[i] = str[i + 1];
}

int main()
{
    setbuf(stdout, NULL);
    // struct passwd *usr = getpwuid(getuid());
    char *home = getenv("HOME");

    while (1)
    {
        char input[100];
        printf("$ ");
        if (!fgets(input, sizeof(input), stdin))
            break;
        // Remove newline
        input[strcspn(input, "\n")] = '\0';

        // Tokenize into argv[]
        // single and double quote handling
        int argc = 0;

        char *argv[20];
        char cmd[100];
        argc = 0;
        int single_q = 0, double_q = 0;

        /*         // filenames for stdout and stderr respectively
                char *command_str = input;
                char *out = NULL, *outE = NULL;
                int offset = 1, offsetE = 1; // default for '>'

                char *p1 = input;
                char *cmd_p = cmd;
         */
        char *p = input;

        char *stdout_redir = NULL;
        char *stderr_redir = NULL;

        // Find redirection operators
        for (*p; *p; ++p)
        {
            if (*p == '>')
            {
                if (p != input && *(p - 1) == '2')
                {
                    // Found 2>
                    *(p - 1) = '\0'; // Terminate command before '2'
                    stderr_redir = p + 1;
                }
                else
                {
                    // Found >
                    *p = '\0'; // Terminate command before '>'
                    stdout_redir = p + 1;
                }
                break; // Only handle one redirection per command for simplicity
            }
        }

        // if out and if outE block...

        // char *p = command_str;
        while (*p)
        {
            // Skip leading spaces
            while (*p == ' ' && !single_q && !double_q)
                p++;
            if (*p == '\0')
                break;

            argv[argc] = p;
            argc++;

            while (*p)
            {
                // double inside single
                if (*p == '\'' && !double_q)
                {
                    single_q = !single_q;
                    remove_char(p, 0);
                    continue;
                }
                // single inside double
                if (*p == '"' && !single_q)
                {
                    double_q = !double_q;
                    remove_char(p, 0);
                    continue;
                }
                // Handle backslash
                if (*p == '\\')
                {
                    if (double_q && (p[1] == '"' || p[1] == '\\' || p[1] == '$' || p[1] == '`'))
                    {
                        remove_char(p, 0);
                        if (*p != '\0')
                            p++;
                        continue;
                    }
                    if (!single_q && !double_q)
                    {
                        remove_char(p, 0);
                        if (*p != '\0')
                            p++;
                        continue;
                    }
                    // skip
                    p++;
                    continue;
                }
                // reached end of string
                if (!single_q && !double_q && *p == ' ')
                {
                    *p = '\0';
                    p++;
                    break;
                }
                p++;
            }
        }
        argv[argc] = NULL;

        /*         // redirect in copy file?
                int saved_stdout = -1, saved_stderr = -1;
                int fd = -1, fdE = -1;
                if (out)
                {
                    fd = open(out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd == -1)
                    {
                        perror("open");
                        // handle error, maybe continue or return
                        continue;
                    }
                    else
                    {
                        saved_stdout = dup(STDOUT_FILENO);
                        if (dup2(fd, STDOUT_FILENO) == -1)
                        {
                            perror("dup2");
                            close(fd);
                            // handle error
                            continue;
                        }
                    }
                }
                if (outE)
                {
                    fdE = open(outE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fdE == -1)
                    {
                        perror("open");
                        continue;
                    }
                    else
                    {
                        saved_stderr = dup(STDERR_FILENO);
                        if (dup2(fd, STDERR_FILENO) == -1)
                        {
                            perror("dup2");
                            close(fdE);
                            continue;
                        }
                    }
                } */

        // Built-in: exit
        if (strcmp(argv[0], "exit") == 0 && (argc == 1 || (argc == 2 && strcmp(argv[1], "0") == 0)))
        {
            /*             if (out)
                        {
                            fflush(stdout);
                            dup2(saved_stdout, STDOUT_FILENO);
                            close(fd);
                            close(saved_stdout);
                        } */
            break;
        }

        // Built-in: echo
        else if (strcmp(argv[0], "echo") == 0)
        {
            for (int i = 1; i < argc; i++)
            {
                printf("%s", argv[i]);
                if (i < argc - 1)
                    printf(" ");
            }
            printf("\n");
        }

        // built-in: pwd
        else if (strcmp(argv[0], "pwd") == 0)
        {
            char cwd[PATH_MAX];
            if (getcwd(cwd, sizeof(cwd)) != NULL)
                printf("%s\n", cwd);
            else
                perror("pwd");
        }

        // built-in: cd
        else if (strcmp(argv[0], "cd") == 0)
        {
            char path[PATH_MAX];
            char *dir = argv[1]; // directory to cd to
            if (argc == 1 || (argv[1] && strcmp(argv[1], "~") == 0))
                dir = home;
            else if (dir && dir[0] == '~')
            {
                // cd ~/sth
                snprintf(path, sizeof(path), "%s%s", home, dir + 1);
                dir = path;
            }
            if (dir && chdir(dir) != 0)
                fprintf(stderr, "cd: %s: No such file or directory\n", dir);
        }

        // Built-in: type
        else if (strcmp(argv[0], "type") == 0 && argc >= 2)
        {
            for (int i = 1; i < argc; i++)
            {
                char *cmd = argv[i];
                if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "echo") == 0 || strcmp(cmd, "type") == 0 || strcmp(cmd, "pwd") == 0 || strcmp(cmd, "cd") == 0)
                {
                    printf("%s is a shell builtin\n", cmd);
                }
                else
                {
                    char *path = getenv("PATH");
                    char *path_copy = strdup(path);
                    bool found = 0;
                    for (char *dir = strtok(path_copy, ":"); dir != NULL; dir = strtok(NULL, ":"))
                    {
                        char fullpath[PATH_MAX];
                        snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, cmd);
                        if (access(fullpath, X_OK) == 0)
                        {
                            printf("%s is %s\n", cmd, fullpath);
                            found = 1;
                            break;
                        }
                    }
                    free(path_copy);
                    if (!found)
                        printf("%s: not found\n", cmd);
                }
            }
        }

        // External command + stdout redirection
        else
        {
            pid_t pid = fork();
            if (pid == 0)
            {
                execvp(argv[0], argv);
                fprintf(stderr, "%s: command not found\n", argv[0]);
                exit(1);
            }
            else if (pid > 0)
                waitpid(pid, NULL, 0);
            else
                perror("fork");
        }
        /*         if (out)
                {
                    fflush(stdout);
                    dup2(saved_stdout, STDOUT_FILENO);
                    close(fd);
                    close(saved_stdout);
                }
                if (outE)
                {
                    fflush(stderr);
                    dup2(saved_stderr, STDERR_FILENO);
                    close(fdE);
                    close(saved_stderr);
                } */
    }
    return 0;
}