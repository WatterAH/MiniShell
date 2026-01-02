#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "lib/shell-history.h"

#define MAX_LINE 1024
#define MAX_ARGS 64
#define VERDE_FUERTE "\033[1;32m"
#define COLOR_RESET "\033[0m"

// Función para parsear comandos
void parse(char *line, char **args)
{
    int i = 0;
    args[i] = strtok(line, " ");
    while (args[i] != NULL)
    {
        i++;
        args[i] = strtok(NULL, " ");
    }
}

// Ejecutar comando simple
void ejecutar_simple(char **args)
{
    pid_t pid = fork();

    if (pid == 0)
    {
        execvp(args[0], args);
        perror("Error");
        exit(1);
    }
    else
    {
        wait(NULL);
    }
}

// Ejecutar comando con pipe
void ejecutar_pipe(char **args1, char **args2)
{
    int fd[2];
    pipe(fd);

    pid_t pid1 = fork();
    if (pid1 == 0)
    {
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);
        execvp(args1[0], args1);
        perror("Error");
        exit(1);
    }

    pid_t pid2 = fork();
    if (pid2 == 0)
    {
        dup2(fd[0], STDIN_FILENO);
        close(fd[1]);
        close(fd[0]);
        execvp(args2[0], args2);
        perror("Error");
        exit(1);
    }

    close(fd[0]);
    close(fd[1]);

    wait(NULL);
    wait(NULL);
}

// Manejar señal SIGINT (Ctrl+C)
void handle_exit(int sig)
{
    disableRawMode();
    printf("\nHasta pronto!\n");
    exit(0);
}

// MAIN (shell)
int main()
{
    signal(SIGINT, handle_exit);
    enableRawMode();

    char line[MAX_LINE];
    char *args1[MAX_ARGS];
    char *args2[MAX_ARGS];
    char cwd[1024];
    char prompt[1100];

    while (1)
    {
        if (getcwd(cwd, sizeof(cwd)) != NULL)
        {
            sprintf(prompt, "%sminishell:%s>%s", VERDE_FUERTE, cwd, COLOR_RESET);
        }
        else
        {
            perror("getcwd() error");
            strcpy(prompt, "minishell>");
        }

        printf("%s ", prompt);
        fflush(stdout);

        readline(prompt, line, MAX_LINE);

        line[strcspn(line, "\n")] = 0;

        if (strlen(line) == 0)
            continue;

        // Salir del shell
        if (strcmp(line, "exit") == 0)
            break;

        // Agregar al historial
        addHistory(line);

        // Buscar pipe
        char *pipe_pos = strchr(line, '|');

        // Comando con pipe
        if (pipe_pos)
        {
            *pipe_pos = '\0';
            pipe_pos++;

            parse(line, args1);
            parse(pipe_pos, args2);

            ejecutar_pipe(args1, args2);
        }
        // Comando sin pipe
        else
        {
            parse(line, args1);

            // Built-in: cd
            if (strcmp(args1[0], "cd") == 0)
            {
                if (args1[1] == NULL)
                {
                    fprintf(stderr, "cd: falta argumento\n");
                }
                else
                {
                    chdir(args1[1]);
                }
                continue;
            }

            ejecutar_simple(args1);
        }
    }

    handle_exit(0);
    return 0;
}
