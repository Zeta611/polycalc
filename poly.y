%code top {
#include <stdio.h>
#include "term.h"
}

%code requires {
#include "ast.h"
}

%start	prgm

%union {
	double	num;
	char	*var;
	ASTNode	*node;
}

%token	<num>	NUM
%token	<var>	VAR

%type	<node>	expr

%left	'+'
%left	'*'

%%

prgm:	  // nothing
	| prgm '\n'
	| prgm expr '\n' {
		printf("AST: ");
		print_node($2);
		putchar('\n');
		printf("VAL: ");
		TermNode *p = eval_node($2);
		print_poly(p);
		free_poly(p);

		putchar('\n');
		free_node($2); }
	;
expr:	  NUM	{ $$ = num_node($1); }
	| VAR	{ $$ = var_node($1); }
	| expr '+' expr	{ $$ = op_node(ADD, $1, $3); }
	| expr '*' expr	{ $$ = op_node(MUL, $1, $3); }
	| '(' expr ')'	{ $$ = $2; }
	;
%%

char *progname;
int lineno = 1;

int main(int argc, char *argv[])
{
	progname = argv[0];
	/* test(); */
	yyparse();
	return 0;
}

int yyerror(char *s)
{
	fprintf(stderr, "%s: %s near line %d\n", progname, s, lineno);
	return 0;
}
