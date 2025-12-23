#include "shim.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

/* shim.c - braindead replacement for readline
 *
 * >>>> This file was written by AI. <<<<
 *
 * Thanks uses GNU Readline for line input. Works fine on desktop, but on the 
 * web, problems: Emscripten won't compile Readline (because it can't compile
 * ncurses) and it doesn't seem like an easy fix.
 *
 * This file is a shim library that does the bare minimum to fill in for 
 * Readline - line editing, session history - without any dependencies.
 *
 * You should only use this file for the web target; it's worse than Readline
 * in every way. I didn't even write it. Who knows what it'll do?
 */

#define MAX_LINE_LENGTH 2048
#define MAX_HISTORY 300

static char* history[MAX_HISTORY];
static int history_count = 0;
static int history_index = 0;

static struct termios orig_termios;

static void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

static void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static void clear_line() {
    printf("\r\33[K");
}

static void redisplay(const char* prompt, const char* buffer, int cursor_pos) {
    clear_line();
    printf("%s%s", prompt, buffer);
    int len = strlen(buffer);
    if (cursor_pos < len) {
        printf("\r");
        int prompt_len = strlen(prompt);
        for (int i = 0; i < prompt_len + cursor_pos; i++) {
            printf("\33[C");
        }
    }
    fflush(stdout);
}

char* readline(const char* prompt) {
    static char buffer[MAX_LINE_LENGTH];
    static char saved_buffer[MAX_LINE_LENGTH];
    int pos = 0;
    int cursor = 0;
    int browsing_history = 0;

    buffer[0] = '\0';
    saved_buffer[0] = '\0';

    printf("%s", prompt);
    fflush(stdout);

    enable_raw_mode();
    history_index = history_count;

    while (1) {
        char c;
        if (read(STDIN_FILENO, &c, 1) != 1) {
            disable_raw_mode();
            return NULL;
        }

        if (c == '\n' || c == '\r') {
            disable_raw_mode();
            printf("\n");
            if (pos > 0) {
                char* result = malloc(pos + 1);
                if (result) {
                    memcpy(result, buffer, pos);
                    result[pos] = '\0';
                }
                return result;
            }
            return strdup("");
        }
        else if (c == 4) { // Ctrl-D (EOF)
            if (pos == 0) {
                disable_raw_mode();
                printf("\n");
                return NULL;
            }
        }
        else if (c == 127 || c == 8) { // Backspace
            if (cursor > 0) {
                memmove(&buffer[cursor - 1], &buffer[cursor], pos - cursor);
                cursor--;
                pos--;
                buffer[pos] = '\0';
                redisplay(prompt, buffer, cursor);
            }
        }
        else if (c == 27) { // Escape sequence
            char seq[3];

            if (read(STDIN_FILENO, &seq[0], 1) != 1) continue;
            if (read(STDIN_FILENO, &seq[1], 1) != 1) continue;

            if (seq[0] == '[') {
                switch (seq[1]) {
                    case 'A': // Up arrow
                        if (history_index > 0) {
                            if (!browsing_history) {
                                strcpy(saved_buffer, buffer);
                                browsing_history = 1;
                            }
                            history_index--;
                            strcpy(buffer, history[history_index]);
                            pos = strlen(buffer);
                            cursor = pos;
                            redisplay(prompt, buffer, cursor);
                        }
                        break;

                    case 'B': // Down arrow
                        if (browsing_history) {
                            if (history_index < history_count - 1) {
                                history_index++;
                                strcpy(buffer, history[history_index]);
                                pos = strlen(buffer);
                                cursor = pos;
                            } else {
                                history_index = history_count;
                                strcpy(buffer, saved_buffer);
                                pos = strlen(buffer);
                                cursor = pos;
                                browsing_history = 0;
                            }
                            redisplay(prompt, buffer, cursor);
                        }
                        break;

                    case 'C': // Right arrow
                        if (cursor < pos) {
                            cursor++;
                            printf("\33[C");
                            fflush(stdout);
                        }
                        break;

                    case 'D': // Left arrow
                        if (cursor > 0) {
                            cursor--;
                            printf("\33[D");
                            fflush(stdout);
                        }
                        break;

                    case '3': // Delete key (ESC[3~)
                        if (read(STDIN_FILENO, &seq[2], 1) == 1 && seq[2] == '~') {
                            if (cursor < pos) {
                                memmove(&buffer[cursor], &buffer[cursor + 1], pos - cursor - 1);
                                pos--;
                                buffer[pos] = '\0';
                                redisplay(prompt, buffer, cursor);
                            }
                        }
                        break;
                }
            }
        }
        else if (c >= 32 && c < 127) { // Printable characters
            if (pos < MAX_LINE_LENGTH - 1) {
                memmove(&buffer[cursor + 1], &buffer[cursor], pos - cursor);
                buffer[cursor] = c;
                cursor++;
                pos++;
                buffer[pos] = '\0';
                redisplay(prompt, buffer, cursor);
            }
        }
    }
}

void add_history(const char* line) {
    if (!line || !*line) return;

    // Don't add duplicate consecutive entries
    if (history_count > 0 && strcmp(history[history_count - 1], line) == 0) {
        return;
    }

    // If we've reached the limit, free the oldest entry and shift
    if (history_count >= MAX_HISTORY) {
        free(history[0]);
        memmove(history, history + 1, (MAX_HISTORY - 1) * sizeof(char*));
        history_count--;
    }

    history[history_count] = strdup(line);
    if (history[history_count]) {
        history_count++;
    }
}

void using_history(void) {
    // Stub - history is always enabled
}

int read_history(const char* filename) {
    // Stub - return success without actually reading
    (void)filename;
    return 0;
}

int write_history(const char* filename) {
    // Stub - return success without actually writing
    (void)filename;
    return 0;
}

int rl_variable_bind(const char* variable, const char* value) {
    // Stub - return success without actually setting variables
    (void)variable;
    (void)value;
    return 0;
}

void rl_initialize(void) {
    // Stub - initialization already happens in readline()
}

int rl_bind_key(int key, rl_command_func_t* func) {
    // Stub - return success without actually binding keys
    (void)key;
    (void)func;
    return 0;
}

int rl_insert(int count, int key) {
    // Stub - not used since we don't actually rebind keys
    (void)count;
    (void)key;
    return 0;
}
