#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include "shell-history.h"

struct termios orig_termios;
char history[MAX_HISTORY][MAX_LINE];
int history_index = 0;

void disableRawMode()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode()
{
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void addHistory(const char *line)
{
    if (strlen(line) > 0)
    {
        strcpy(history[history_index % MAX_HISTORY], line);
        history_index++;
    }
}

void readline(char *prompt, char *buffer, int size)
{
    int pos = 0;
    int h_ptr = history_index; // Empezamos desde el final del historial
    char c;

    memset(buffer, 0, size);

    while (1)
    {
        c = getchar();

        if (c == 27)
        {              // Es una secuencia de escape (ESC)
            getchar(); // saltar '['
            char seq = getchar();

            if (seq == 'A')
            { // FLECHA ARRIBA
                if (h_ptr > 0 && h_ptr > history_index - MAX_HISTORY)
                {
                    h_ptr--;
                    // Limpiar línea actual en la terminal
                    printf("\r\033[K%s %s", prompt, history[h_ptr % MAX_HISTORY]);
                    strcpy(buffer, history[h_ptr % MAX_HISTORY]);
                    pos = strlen(buffer);
                }
            }
            else if (seq == 'B')
            { // FLECHA ABAJO
                if (h_ptr < history_index - 1)
                {
                    h_ptr++;
                    printf("\r\033[K%s %s", prompt, history[h_ptr % MAX_HISTORY]);
                    strcpy(buffer, history[h_ptr % MAX_HISTORY]);
                    pos = strlen(buffer);
                }
                else
                {
                    // Volver a línea vacía si bajamos del todo
                    h_ptr = history_index;
                    printf("\r\033[K%s ", prompt);
                    memset(buffer, 0, size);
                    pos = 0;
                }
            }
        }
        else if (c == 127 || c == 8)
        { // BACKSPACE
            if (pos > 0)
            {
                pos--;
                buffer[pos] = '\0';
                printf("\b \b"); // Retrocede, escribe espacio, retrocede
            }
        }
        else if (c == '\n')
        { // ENTER
            buffer[pos] = '\0';
            printf("\n");
            break;
        }
        else if (pos < size - 1)
        { // CARÁCTER NORMAL
            buffer[pos++] = c;
            putchar(c);
        }
    }
}