#include "debugger.h"

#define TMP_DIR "/tmp/"

WINDOW **mem_cells;
int mem_cell_len, mem_cell_start;
const int mem_cell_width = 12;
const int mem_cell_spacing = 2;

WINDOW *code, *mem, *code_inside, *help_win;

const char *help_text =
"Command line arguments:\n"
"-h - prints this message\n"
"-i - sets the input file for the script (otherwise you get asked to input each character)\n"
// TODO: add this option
"-o - sets the output file (otherwise it gets printed in the debugger)\n"
"\n"
"Inside the editor:\n"
"Enter - next instruction\n"
"q - exit\n"
"Mouse wheel - vertical scroll the code\n"
"Shift + Mouse wheel - horizontal scroll the code\n"
"l - run until loop is done\n";

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
        wrefresh(mem);
    }

    touchwin(mem);
    mem_cell_len = (x - 2) / (mem_cell_width + mem_cell_spacing);
    mem_cell_start = (x - 2) % (mem_cell_width + mem_cell_spacing) / 2;
    mem_cells = malloc(sizeof(WINDOW *) * mem_cell_len);
    int i;
    for (i = 0; i < mem_cell_len; ++ i) {
        mem_cells[i] = derwin(mem, getmaxy(mem) - 2, mem_cell_width, 1, i * (mem_cell_width + mem_cell_spacing) + mem_cell_start);
        box(mem_cells[i], 0, 0);
    }
    wrefresh(mem);

    // help_win = newwin(sizeof(help_text) / sizeof(*help_text), 50, 0, 0);
    // refresh();
    // for (i = 0; i < sizeof(help_text) / sizeof(*help_text); ++ i) {
    //     mvwaddstr(help_win, i, 1, help_text[i]);
    // }
    // wrefresh(help_win);
}

void print_memory(interpreter_t *interp, int start_addr) {
    int maxy = getmaxy(mem_cells[0]);

    int i;
    for (i = 0; i < mem_cell_len; ++ i) {
        int addr = start_addr + i;
        addr += (addr < 0 ? interp->memory_len : 0);
        mvwprintw(mem_cells[i], 1, 2 + 2, "%05d", addr);
        mvwprintw(mem_cells[i], maxy - 2, 2 + 2, "% 4d", interp->memory[interp->mem_y][addr]);
        mvwaddch(mem_cells[i], 1, 2, interp->mem_x == start_addr + i ? '*' : ' ');
    }
    touchwin(mem);
    wrefresh(mem);
}

span_t *split_code_lines(interpreter_t *interp, int *line_count) {
    int lines = 1;
    size_t i = 0;
    while (i < interp->code_len) {
        if (interp_char_at(interp, i) == '\n' || interp_char_at(interp, i) == EOF) {
            ++ lines;
        }
        ++ i;
    }
    fprintf(logfile, "lines: %d\n", lines);

    span_t *line_spans = malloc(sizeof(span_t) * lines);
    size_t top = 0;

    size_t start;
    start = i = 0;
    while (i < interp->code_len) {
        if (interp_char_at(interp, i) == '\n' || interp_char_at(interp, i) == EOF) {
            line_spans[top] = (span_t){start, i - start};
            ++ top;
            start = i + 1;
        }
        ++ i;
    }
    line_spans[top] = (span_t){start, i - start};

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

    signal(SIGINT, quit);
}

void die(const char *msg, int error) {
    fprintf(stderr, msg);
    quit(error);
}

void quit(int sig) {
    nocbreak();
    echo();

    intrflush(stdscr, TRUE);
    keypad(stdscr, FALSE);

    endwin();
    // exit_curses(sig);
    exit(sig);
}

FILE *infile, *outfile;
int is_infile_tmp, is_outfile_tmp;
void handle_args(int argc, char *argv[]) {
    int i;
    for (i = 1; i < argc; ++ i) {
        if (!strcmp(argv[i], "-h")) {
            die(help_text, 0);
        } else if (!strcmp(argv[i], "-i")) {
            if (infile) die("Cannot read from multiple files.", -1);

            if (i + 1 >= argc) die("-i requires an argument.", -1);

            infile = fopen(argv[i + 1], "r");
            if (!infile) die("Cannot open file specified for -i.", -1);

            ++ i;
        } else if (!strcmp(argv[i], "-o")) {
            if (outfile) die("Cannot write to multiple files.", -1);

            if (i + 1 >= argc) die("-i requires an argument.", -1);

            outfile = fopen(argv[i + 1], "r");
            if (!outfile) die("Cannot open file specified for -i.", -1);

            ++ i;
        }
        
        if (source) {
            die("Cannot debug more than one file.\n", -1);
        }
        source = fopen(argv[i], "r");
        if (!source) {
            die("Cannot open file.\n", -1);
        }
    }

    // exit(source);
    if (!source) {
        die("Specify a source file.\n", -1);
    }
}

void loop(void) {
    interpreter_t *interp = make_interpreter(source, infile, outfile, 300);
    int len;
    span_t *spans = split_code_lines(interp, &len);
    print_code(interp, spans, len, 0, 0);
    wrefresh(code_inside);

    int code_inside_x  [[maybe_unused]], code_inside_y;
    getmaxyx(code_inside, code_inside_y, code_inside_x);

    int vscroll, hscroll;
    vscroll = hscroll = 0;

    int mem_scroll = 0;

    mousemask(BUTTON4_PRESSED | BUTTON5_PRESSED, NULL);

    while (1) {
        int c = wgetch(stdscr);
        switch (c) {
            case 'q': {
                exit(0);
            } break;
            case '\n': {
                // NOTE: this is done because we want to have a notification that we are reading input
                if (is_infile_tmp && interp_current_char(interp) == ',') {
                    mvwaddch(code_inside, 1, 1, '*');
                    int in = getchar();
                    // TODO: notify user that the debugger is wating for input
                    fputc(in, infile);
                    mvwaddch(code_inside, 1, 1, ' ');
                    fprintf(logfile, "read %d\n", in);
                    fflush(logfile);
                    fseek(infile, -1, SEEK_CUR);
                }
                fprintf(outfile, "Entering the big fuckup\n");
                fflush(outfile);
                enum interp_action action = interpreter_step(interp);
                // TODO: handle action == DONE
                if (interp->mem_x < mem_scroll || interp->mem_x > mem_scroll + mem_cell_len) {
                    if (action == MOVE_RIGHT) {
                        ++ mem_scroll;
                    } else if (action == MOVE_LEFT) {
                        -- mem_scroll;
                    }
                }

                print_memory(interp, mem_scroll);
                wmove(code_inside, interp->pos.line_no - vscroll, interp->pos.col_no - hscroll);
                wrefresh(code_inside);
            } break;
            case 'l': {
                if (interp->loop_depth <= 0) {
                    continue;
                }

                while (interpreter_step(interp) != EXIT_LOOP);

                mem_scroll = interp->mem_x - mem_cell_len / 2;

                print_memory(interp, mem_scroll);
                wmove(code_inside, interp->pos.line_no - vscroll, interp->pos.col_no - hscroll);
                wrefresh(code_inside);
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
}

int main(int argc, char *argv[]) {
    logfile = fopen("log", "w");
    
    handle_args(argc, argv);
    
    char tmp_template[] = TMP_DIR "bf_debugger_XXXXXX";
    if (!infile) {
        infile = fdopen(mkstemp(tmp_template), "w+");
        strcpy(tmp_template, TMP_DIR "bf_debugger_XXXXXX");
        is_infile_tmp = 1;
    }
    if (!outfile) {
        outfile = fdopen(mkstemp(tmp_template), "w+");
        is_outfile_tmp = 1;
    }
    fprintf(infile, "yo mama.\n");
    fflush(infile);

    init();

    draw_windows();
    
    loop();

    quit(0);
}