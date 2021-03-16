%{
#include <stdlib.h>
#include "poly.h"
#define YYSTYPE double
%}

%token	NUM VAR
%token	PLUS TIMES
%token	LPAR RPAR
%token	EOL

%left	PLUS
%left	TIMES
%%
prgm:	  /* nothing */
	| prgm EOL
	| prgm expr EOL
	;
expr:	  NUM
	| VAR
	| expr PLUS expr
	| expr TIMES expr
	| LPAR expr RPAR
	;
%%	/* end of grammar */
#include <stdio.h>

char *progname;
int lineno = 1;

int main(int argc, char *argv[])
{
	progname = argv[0];
	yyparse();
	return 0;
}

int yyerror(char *s)
{
	fprintf(stderr, "%s: %s near line %d\n", progname, s, lineno);
	return 0;
}
