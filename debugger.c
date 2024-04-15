#include "debugger.h"

// int mem_width;
// const int mem_cell_width = 9;

WINDOW *code, *mem, *code_inside;

FILE *source = NULL;

FILE *logfile;

#define CODE_HEIGHT   4
#define MEMORY_HEIGHT 1
void draw_windows(void) {
    int x, y;

    getmaxyx(stdscr, y, x);

    int mem_y = y - (MEMORY_HEIGHT * y / (CODE_HEIGHT + MEMORY_HEIGHT));

    if (!code) {
        code = newwin(mem_y - 1, x - 1, 0, 0);
        code_inside = derwin(code, mem_y - 3, x - 5, 1, 2);
        scrollok(code_inside, TRUE);
        refresh();
        box(code, 0, 0);
        wrefresh(code);
    }

    if (!mem) {
        mem = newwin(y - mem_y + 1, x - 1, mem_y - 1, 0);
        refresh();
        box(mem, 0, 0);
        wrefresh(mem);
    }
}

span_t *split_code_lines(interpreter_t *interp, int *line_count) {
    int lines = 0;
    size_t i = 0;
    while (i < interp->code_len) {
        if (interp_char_at(interp, i) == '\n') {
            ++ lines;
        }
        ++ i;
    }

    span_t *line_spans = malloc(sizeof(span_t) * lines);
    size_t top = 0;

    size_t start;
    start = i = 0;
    while (i < interp->code_len) {
        if (interp_char_at(interp, i) == '\n') {
            line_spans[top] = (span_t){start, i - start};
            ++ top;
            start = i + 1;
        }
        ++ i;
    }

    *line_count = lines;
    return line_spans;
}

void print_code(interpreter_t *interp, span_t *spans, int len, int vscroll, int hscroll) {
    wclear(code_inside);
    
    int maxx = getmaxx(code_inside);
    int maxy = getmaxy(code_inside);
    int i;

    int scroll_start;
    int line_len;
    int lines = len - vscroll < maxy ? len - vscroll : maxy;

    scrollok(code_inside, FALSE);
    for (i = vscroll; i < vscroll + lines; ++ i) {
        if (hscroll > spans[i].len) {
            continue;
        }

        scroll_start = spans[i].start + hscroll;
        line_len = maxx < spans[i].len - hscroll ? maxx : spans[i].len - hscroll;
        mvwaddnstr(code_inside, i - vscroll, 0, interp->code + scroll_start, line_len);
    }
    scrollok(code_inside, TRUE);
}

void print_code_line(interpreter_t *interp, span_t line, int hscroll, int y, int x) {
    if (hscroll > line.len) {
        return;
    }

    int old_x, old_y;
    getyx(code_inside, old_y, old_x);

    int maxx = getmaxx(code_inside);
    
    int scroll_start = line.start + hscroll;
    int line_len = maxx < line.len - hscroll ? maxx : line.len - hscroll;
    scrollok(code_inside, FALSE);
    mvwaddnstr(code_inside, y, x, interp->code + scroll_start, line_len);
    scrollok(code_inside, TRUE);
    wmove(code_inside, old_y, old_x);
}

void init(void) {
    setlocale(LC_CTYPE, "en_US.UTF-8");

    initscr();
    cbreak();
    noecho();

    intrflush(stdscr, FALSE);
    keypad(stdscr, TRUE);

    curs_set(2);

    // addch(ACS_HLINE);

    signal(SIGINT, quit);
}

void die(char *msg, int error) {
    fprintf(stderr, msg);
    quit(error);
}

void quit(int sig) {
    nocbreak();
    echo();

    exit_curses(sig);
}

void handle_args(int argc, char *argv[]) {
    int i;
    for (i = 1; i < argc; ++ i) {
        // TODO: other options
        if (source) {
            die("Cannot debug more than one file.\n", -1);
        }
        source = fopen(argv[i], "r");
        if (!source) {
            die("Cannot open file.\n", -1);
        }
    }

    if (!source) {
        die("Specify a source file.\n", -1);
    }
}

int main(int argc, char *argv[]) {
    logfile = fopen("log", "w");
    
    handle_args(argc, argv);

    init();

    draw_windows();

    interpreter_t *interp = make_interpreter(source, stdin, stdout, 100);
    int len;
    span_t *spans = split_code_lines(interp, &len);
    print_code(interp, spans, len, 0, 0);
    wrefresh(code_inside);

    int code_inside_x, code_inside_y;
    getmaxyx(code_inside, code_inside_y, code_inside_x);

    int vscroll, hscroll;
    vscroll = hscroll = 0;
    
    mousemask(BUTTON4_PRESSED | BUTTON5_PRESSED, NULL);
    
    while (1) {
        int c = wgetch(stdscr);
        switch (c) {
            case 'q': {
                exit(0);
            } break;
            case '\n': {
                interpreter_step(interp);
                wmove(code_inside, interp->pos.line_no - vscroll, interp->pos.col_no - hscroll);
                wrefresh(code_inside);
                // update_memory(interp);
            } break;
            case KEY_MOUSE: {
                MEVENT ev;

                if (getmouse(&ev) != OK) break;

                if (ev.bstate == BUTTON5_PRESSED && vscroll + code_inside_y < len) {
                    ++ vscroll;
                    wscrl(code_inside, 1);
                    print_code_line(interp, spans[vscroll + code_inside_y - 1], hscroll, code_inside_y - 1, 0);
                } else if (ev.bstate == BUTTON4_PRESSED && vscroll > 0) {
                    -- vscroll;
                    wscrl(code_inside, -1);
                    print_code_line(interp, spans[vscroll], hscroll, 0, 0);
                }

                wrefresh(code_inside);
            } break;
            // TODO: could probably implement the same optimization as wscrl
            case 396: {
                ++ hscroll;
                print_code(interp, spans, len, vscroll, hscroll);
                wrefresh(code_inside);
            } break;
            case 398: {
                if (hscroll <= 0) break;

                -- hscroll;
                print_code(interp, spans, len, vscroll, hscroll);
                wrefresh(code_inside);
            } break;
            default: {
                fprintf(logfile, "Unknown key %d\n", c);
            }
        }
    }

    quit(0);
}