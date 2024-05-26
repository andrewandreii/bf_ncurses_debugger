libs=ncurses

LFLAGS=`pkgconf --libs $(libs)`
sources=$(wildcard *.c interpreter/*.c)
objs=$(sources:.c=.o)

CC=gcc
CFLAGS=-Wall -c `pkgconf --cflags $(libs)`
output=debugger

all: debugger
debugger: $(objs)
	$(CC) $(objs) $(LFLAGS) -o $(output)
 
%.o: %.c
	$(CC) $^ $(CFLAGS) -o $@

clean:
	rm $(objs)
