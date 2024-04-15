#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum interpreter_state_type {
    NORMAL, DONE
};

typedef struct {
    size_t line_no, col_no, pos;
} code_pos_t;

#define MAX_LOOP_DEPTH 100
typedef struct {
    char *code;
    size_t code_len;
    code_pos_t pos;

    enum interpreter_state_type state;

    code_pos_t loop_pos[MAX_LOOP_DEPTH];
    int loop_depth;

    char **memory;
    size_t memory_len;
    size_t memory_height;
    size_t mem_x;
    size_t mem_y;

    FILE *input, *output;
} interpreter_t;

#define interp_next_char(interp) \
    if ((interp)->code[(interp)->pos.pos] == '\n') { \
        ++ (interp)->pos.line_no; \
        (interp)->pos.col_no = 0; \
    } else { \
        ++ (interp)->pos.col_no; \
    } \
    ++(interp)->pos.pos;

#define interp_current_char(interp) (interp)->code[(interp)->pos.pos]

#define interp_char_at(interp, i) (interp)->code[i]

#define interp_code_eof(interp) ((interp)->pos.pos >= (interp)->code_len)

interpreter_t *make_interpreter(FILE *source, FILE *input, FILE *output, size_t memory_len);
void memory_dump(interpreter_t *i, FILE *stream);
void interpreter_step(interpreter_t *i);

#endif