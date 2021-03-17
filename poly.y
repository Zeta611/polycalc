%{
#include <stdio.h>
#include "poly.h"
%}

%start	prgm

%union {
	double	num;
	Node	*node;
}

%token	<num>	NUM
%token		VAR
%token		LPAR RPAR
%token		EOL

%type	<node>	expr

%left	PLUS
%left	TIMES

%%

prgm:	  // nothing
	| prgm EOL
	| prgm expr EOL	{
		printf("VAL: %lf\n", eval_node($2));
		printf("AST: ");
		debug_node($2);
		putchar('\n');
		putchar('\n');
		free_node($2); }
	;
expr:	  NUM	{ $$ = num_node($1); }
	| VAR
	| expr PLUS expr	{ $$ = op_node(SUM, $1, $3); }
	| expr TIMES expr	{ $$ = op_node(MULT, $1, $3); }
	| LPAR expr RPAR	{ $$ = $2; }
	;
%%

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
