libs=ncurses panel

LFLAGS=`pkgconf --libs $(libs)`
sources=$(wildcard *.c interpreter/*.c)
objs=$(sources:.c=.o)

CC=gcc
CFLAGS=-Wall -c `pkgconf --cflags --std=c23 $(libs)`
output=debugger

all: debugger
debugger: $(objs)
	$(CC) $(objs) $(LFLAGS) -o $(output)
 
%.o: %.c
	$(CC) $^ $(CFLAGS) -o $@

clean:
	rm $(objs)