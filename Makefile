YFLAGS = -d
OBJS = poly.o lex.o

poly:	$(OBJS)
	cc $(OBJS) -ly -ll -o poly

poly.o:	poly.h

lex.o:	x.tab.h

x.tab.h:	y.tab.h
	-cmp -s x.tab.h y.tab.h || cp y.tab.h x.tab.h

clean:
	rm -f $(OBJS) [xy].tab.[ch]
