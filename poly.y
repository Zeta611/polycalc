%code top {
#include <stdio.h>
#include "term.h"
}

%code requires {
#include "ast.h"
}

%start	prgm

%union {
	long	inum;
	double	rnum;
	char	*var;
	ASTNode	*node;
}

%token	<rnum>	RNUM
%token	<inum>	INUM
%token	<var>	VAR

%type	<node>	expr

%left	'+' '-'
%left	'*' '/'
%right	'^'

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
expr:	  INUM	{ $$ = inum_node($1); }
	| RNUM	{ $$ = rnum_node($1); }
	| VAR	{ $$ = var_node($1); }
	| expr '+' expr	{ $$ = op_node(ADD, $1, $3); }
	| expr '-' expr	{ $$ = op_node(SUB, $1, $3); }
	| expr '*' expr	{ $$ = op_node(MUL, $1, $3); }
	| expr '/' expr	{ $$ = op_node(DIV, $1, $3); }
	| expr '^' expr	{ $$ = op_node(POW, $1, $3); }
	| expr expr %prec '*'	{ $$ = op_node(MUL, $1, $2); }
	| '-' expr	{ $$ = op_node(NEG, NULL, $2); }
	| '(' expr ')'	{ $$ = $2; }
	;
%%

char *progname;
int lineno = 1;

static void test(void);

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

static void test(void)
{
	printf("TESTING\n");

	// 2xy^2 + 5y + 9
	TermNode *p1 = icoeff_term(2);
	p1->u.vars = var_term("x", 1);
	p1->u.vars->next = var_term("y", 2);
	p1->next = icoeff_term(5);
	p1->next->u.vars = var_term("y", 1);
	p1->next->next = icoeff_term(9);

	// x^2 + 3xy^2 + x + 1
	TermNode *p2 = icoeff_term(1);
	p2->u.vars = var_term("x", 2);
	p2->next = icoeff_term(3);
	p2->next->u.vars = var_term("x", 1);
	p2->next->u.vars->next = var_term("y", 2);
	p2->next->next = icoeff_term(1);
	p2->next->next->u.vars = var_term("x", 1);
	p2->next->next->next = icoeff_term(1);

	printf("P1: \n");
	print_poly(p1);

	printf("P2: \n");
	print_poly(p2);

	printf("P1 * P2: \n");
	mul_poly(&p1, p2); // `p2` points to garbage henceforth.
	print_poly(p1);
	free_poly(p1);
}
