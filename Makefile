CC = gcc
CFLAGS = -O0 -Wall -Wextra -Wpedantic -std=c17
YFLAGS = -d
LDFLAGS = -ly -ll -lm
OBJS = poly.o lex.o ast.o term.o rel.o

poly:	$(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o poly

poly.o:	ast.h term.h

lex.o:	x.tab.h

ast.o:	ast.h rel.h term.h

term.o:	term.h

rel.o:	rel.h term.h

x.tab.h:	y.tab.h
	-cmp -s x.tab.h y.tab.h || cp y.tab.h x.tab.h

clean:
	rm -f $(OBJS) [xy].tab.[ch]
