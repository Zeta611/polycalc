CC = gcc
CFLAGS = -O0 -Wall -Wextra -Wpedantic -std=c17
YFLAGS = -d
LDFLAGS = -ly -ll
OBJS = poly.o lex.o node.o

poly:	$(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o poly

poly.o:	poly.h

lex.o:	x.tab.h

node.o:	poly.h

x.tab.h:	y.tab.h
	-cmp -s x.tab.h y.tab.h || cp y.tab.h x.tab.h

clean:
	rm -f $(OBJS) [xy].tab.[ch]
