#ifndef HISTORY_H
#define HISTORY_H

#define MAX_HISTORY 10
#define MAX_LINE 1024

void enableRawMode();
void disableRawMode();
void addHistory(const char *line);
void readline(char *prompt, char *buffer, int size);

#endif