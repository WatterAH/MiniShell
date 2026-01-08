#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
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

//  <, >, >>, <<
void gestionar_redirecciones(char **args)
{
    for (int i = 0; args[i] != NULL; i++)
    {

        if (strcmp(args[i], ">") == 0)
        {
            args[i] = NULL;
            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            dup2(fd, STDOUT_FILENO);
            close(fd);
            return;
        }
        // Redirección de salida: >>
        else if (strcmp(args[i], ">>") == 0)
        {
            args[i] = NULL;
            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            dup2(fd, STDOUT_FILENO);
            close(fd);
            return;
        }
        // Redirección de entrada: <
        else if (strcmp(args[i], "<") == 0)
        {
            args[i] = NULL;
            int fd = open(args[i + 1], O_RDONLY);
            if (fd == -1)
            {
                perror("Error al abrir archivo de entrada");
                exit(1);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
        // HereDoc: << (Entrada manual hasta delimitador)
        else if (strcmp(args[i], "<<") == 0)
        {
            args[i] = NULL;
            char *delim = args[i + 1];
            char buffer[1024];
            int fd_temp = open(".temp_heredoc", O_RDWR | O_CREAT | O_TRUNC, 0644);

            // Lectura de la entrada hasta encontrar el delimitador
            while (1)
            {
                write(STDOUT_FILENO, "heredoc> ", 9);
                int n = read(STDIN_FILENO, buffer, 1024);
                if (n <= 0)
                    break;
                buffer[n] = '\0';
                if (strncmp(buffer, delim, strlen(delim)) == 0 && buffer[strlen(delim)] == '\n')
                    break;
                write(fd_temp, buffer, n);
            }
            close(fd_temp);

            // Redirigimos el archivo temporal a la entrada estandar
            fd_temp = open(".temp_heredoc", O_RDONLY);
            dup2(fd_temp, STDIN_FILENO);
            close(fd_temp);
        }
    }
}

// Ejecutar comando simple (modificado para soportar redirecciones)
void ejecutar_simple(char **args)
{
    pid_t pid = fork();

    if (pid == 0)
    {
        gestionar_redirecciones(args); // Aplicar redirecciones
        execvp(args[0], args);
        fprintf(stderr, "minishell: comando no encontrado: %s\n", args[0]);
        exit(127);
    }
    else
    {
        wait(NULL);
    }
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

    char line[MAX_LINE];
    char line_copy[MAX_LINE]; // Copia auxiliar para no dañar la original
    char cwd[1024];
    char prompt[1100];
    char *args[MAX_ARGS];

    while (1)
    {
        enableRawMode();

        // 0. Usuario
        char *user = getenv("USER");
        if (user == NULL)
            user = "user";

        // 1. Mostrar Prompt
        if (getcwd(cwd, sizeof(cwd)) != NULL)
            sprintf(prompt, "%sminishell@%s:%s>%s", VERDE_FUERTE, user, cwd, COLOR_RESET);
        else
            strcpy(prompt, "minishell>");

        printf("%s ", prompt);
        fflush(stdout);

        // 2. Leer línea
        readline(prompt, line, MAX_LINE);
        line[strcspn(line, "\n")] = 0; // Quitar salto de línea

        disableRawMode();

        if (strlen(line) == 0)
            continue;

        // 3. Comandos internos prioritarios
        if (strcmp(line, "exit") == 0)
            break;

        addHistory(line); // Guardar en historial

        // 4. Procesamiento de tuberías (Pipes)
        // Primero contamos y separamos los comandos por '|'
        char *comandos_pipe[MAX_ARGS];
        int num_cmds = 0;

        strcpy(line_copy, line); // Usamos copia porque strtok destruye el string
        comandos_pipe[num_cmds] = strtok(line_copy, "|");
        while (comandos_pipe[num_cmds] != NULL)
        {
            num_cmds++;
            comandos_pipe[num_cmds] = strtok(NULL, "|");
        }

        // CASO A: Comando único (sin pipes)
        if (num_cmds == 1)
        {
            parse(line, args);

            // Built-in: cd
            if (strcmp(args[0], "cd") == 0)
            {
                if (args[1] == NULL)
                    fprintf(stderr, "cd: falta argumento\n");
                else if (chdir(args[1]) != 0)
                    perror("cd failed");
                continue;
            }

            ejecutar_simple(args);
        }
        // CASO B: Múltiples comandos (con pipes)
        else
        {
            int i;
            int pipefd[2];
            int fd_in = 0; // Entrada inicial es el teclado (STDIN)

            for (i = 0; i < num_cmds; i++)
            {
                // Parsear el comando actual del array
                parse(comandos_pipe[i], args);

                // Crear pipe (excepto para el último comando)
                if (i < num_cmds - 1)
                {
                    pipe(pipefd);
                }

                pid_t pid = fork();

                if (pid == 0) // HIJO
                {
                    // Si no es el primero, redirigir entrada desde el anterior
                    if (fd_in != 0)
                    {
                        dup2(fd_in, STDIN_FILENO);
                        close(fd_in);
                    }
                    // Si no es el último, redirigir salida al pipe actual
                    if (i < num_cmds - 1)
                    {
                        dup2(pipefd[1], STDOUT_FILENO);
                        close(pipefd[1]);
                        close(pipefd[0]); // El hijo no lee de su propio pipe de salida
                    }

                    gestionar_redirecciones(args); // Aplicar <, >, >> dentro del pipe
                    execvp(args[0], args);
                    perror("Error execvp");
                    exit(1);
                }
                else // PADRE
                {
                    wait(NULL); // Esperar al hijo

                    // Cerrar escritura del pipe actual en el padre
                    if (i < num_cmds - 1)
                    {
                        close(pipefd[1]);
                    }

                    // Cerrar la lectura anterior (ya la usó el hijo)
                    if (fd_in != 0)
                    {
                        close(fd_in);
                    }

                    // Guardar la lectura actual para el siguiente hijo
                    if (i < num_cmds - 1)
                    {
                        fd_in = pipefd[0];
                    }
                }
            }
        }
    }

    handle_exit(0);
    return 0;
}