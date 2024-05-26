#include <stdio.h>
#include <stdlib.h>

#include "interpreter.h"

int main1() {
    FILE *source = fopen("interpreter/bf_test.bf", "r");
    FILE *input = fopen("interpreter/input", "r");

    interpreter_t *i = make_interpreter(source, input, stdout, 200);

    while (i->state != DONE) {
        scanf("%*c");
        printf("running: %c; pos: (%lu, %lu)\n", i->code[i->pos.pos], i->pos.line_no, i->pos.col_no);
        interpreter_step(i);
        memory_dump(i, stdout);
    }

    memory_dump(i, stdout);

    return 0;
}