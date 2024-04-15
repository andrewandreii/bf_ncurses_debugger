#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <stdlib.h>
#include <stdio.h>

#include <ncurses.h>
// #include <panel.h>

#include <signal.h>

#include <locale.h>

#include "interpreter/interpreter.h"

typedef struct {
    size_t start, len;
} span_t;

void init(void);
void quit(int sig);

#endif