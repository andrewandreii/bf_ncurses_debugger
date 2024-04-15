#include "interpreter.h"

// TODO: filter out unnecessary chars
interpreter_t *make_interpreter(FILE *source, FILE *input, FILE *output, size_t memory_len) {
    interpreter_t *i = malloc(sizeof(interpreter_t));
    
    fseek(source, 0, SEEK_END);

    i->code_len = ftell(source);
    i->code = malloc(i->code_len);
    i->pos = (code_pos_t){0, 0, 0};

    fseek(source, 0, SEEK_SET);
    fread(i->code, i->code_len, 1, source);

    i->memory = malloc(sizeof(char *));
    i->memory_len = memory_len;
    i->memory_height = 1;
    i->memory[0] = calloc(1, memory_len);

    i->mem_y = 0;
    i->mem_x = 0;

    memset(i->loop_pos, -1, sizeof(*(i->loop_pos)) * MAX_LOOP_DEPTH);
    i->loop_depth = 0;
    i->state = NORMAL;

    i->input = input;
    i->output = output;

    return i;
}

// TODO: preprocess code for faaster execution
// if debugging don't preprocess, otherwise do
// void preprocess(interpreter_t *intrp);

void memory_dump(interpreter_t *i, FILE *stream) {
    size_t j, k;
    for (j = 0; j < i->memory_height; ++ j) {
        fprintf(stream, "[%d] = ", j);
        for (k = 0; k < i->memory_len; ++ k) {
            fprintf(stream, "%c[%d; %lu] ", (i->mem_x == k && i->mem_y == j) ? '*' : ' ', i->memory[j][k], k);
        }
        fputc('\n', stream);
    }
}

#define interp_next_char(interp) \
    if ((interp)->code[(interp)->pos.pos] == '\n') { \
        ++ (interp)->pos.line_no; \
        (interp)->pos.col_no = 0; \
    } else { \
        ++ (interp)->pos.col_no; \
    } \
    ++(interp)->pos.pos;

#define interp_current_char(interp) (interp)->code[(interp)->pos.pos]

#define interp_code_eof(interp) ((interp)->pos.pos >= (interp)->code_len)

void interpreter_step(interpreter_t *i) {
    if (interp_code_eof(i)) {
        i->state = DONE;
        return;
    }
    
    switch (interp_current_char(i)) {
        case '+': {
            ++ i->memory[i->mem_y][i->mem_x];
        } break;
        case '-': {
            -- i->memory[i->mem_y][i->mem_x];
        } break;
        case '>': {
            if (i->mem_x >= i->memory_len - 1) {
                i->mem_x = -1;
            }
            ++ i->mem_x;
        } break;
        case '<': {
            if (i->mem_x <= 0) {
                i->mem_x = i->memory_len;
            }
            -- i->mem_x;
        } break;
        case '[': {
            if (i->memory[i->mem_y][i->mem_x]) {
                i->loop_pos[i->loop_depth] = i->pos;
                ++ i->loop_depth;
            } else {
                while (!interp_code_eof(i) && interp_current_char(i) != ']') {
                    interp_next_char(i);
                }
                interp_next_char(i);
            }
        } break;
        case ']': {
            if (!i->memory[i->mem_y][i->mem_x]) {
                -- i->loop_depth;
            } else {
                i->pos = i->loop_pos[i->loop_depth - 1];
            }
        } break;
        case ',': {
            char c = fgetc(i->input);
            if (feof(i->input)) {
                i->memory[i->mem_y][i->mem_x] = 0;
            } else {
                i->memory[i->mem_y][i->mem_x] = c;
            }
        } break;
        case '.': {
            fputc(i->memory[i->mem_y][i->mem_x], i->output);
        } break;
    }

    interp_next_char(i);

    while (!interp_code_eof(i) && strchr("+-><[],.", interp_current_char(i)) == NULL) {
        interp_next_char(i);
    }
}