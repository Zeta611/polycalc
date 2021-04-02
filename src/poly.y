%code top {
#include "term.h"
#include <stdio.h>
#define set_nlist(n) (n)->next = nlist; nlist = (n)
}

%code requires {
#include "ast.h"
#include "rel.h"
}

%parse-param { ASTNode *nlist }

%start	prgm

%union {
	long	inum;
	double	rnum;
	char	*var;
	Rel	rel;
	ASTNode	*node;
}

%token	<rnum>	RNUM
%token	<inum>	INUM
%token	<var>	VAR
%token	<rel>	REL

%type	<node>	atom expt neg mult poly rels expr

%%

prgm:	  // nothing
	| prgm '\n'
	| prgm error '\n'	{ nlist = NULL; }
	| prgm expr '\n' {
		printf("AST: ");
		print_node($2);
		putchar('\n');
		if ($2->type == REL_NODE) {
			RelNode *r;
			if ((r = eval_rel($2))) {
				printf("REL: ");
				print_rel(r);
				putchar('\n');
				free_rel(r);
			}
		} else {
			TermNode *p;
			if ((p = eval_poly($2))) {
				printf("VAL: ");
				print_poly(p);
				putchar('\n');
				free_poly(p);
			}
		}
		putchar('\n');
		free_node(nlist);
		nlist = NULL; }
	;
expr:	  poly
	| rels
	;
rels:	  poly REL poly	{ $$ = rel_node($2, $1, $3); set_nlist($$); }
	| poly REL poly '&' rels {
		$$ = rel_node($2, $1, $3);
		set_nlist($$);
		$$->u.reldat.next = $5; }
	;
poly:	  mult
	| poly '+' mult	{ $$ = op_node(ADD, $1, $3); set_nlist($$); }
	| poly '-' mult	{ $$ = op_node(SUB, $1, $3); set_nlist($$); }
	;
mult:	  neg
	| mult '*' neg	{ $$ = op_node(MUL, $1, $3); set_nlist($$); }
	| mult '/' neg	{ $$ = op_node(DIV, $1, $3); set_nlist($$); }
	| mult expt	{ $$ = op_node(MUL, $1, $2); set_nlist($$); }
	;
neg:	  expt
	| '-' neg	{ $$ = op_node(NEG, $2, NULL); set_nlist($$); }
	;
expt:	  atom
	| atom '^' neg	{ $$ = op_node(POW, $1, $3); set_nlist($$); }
	;
atom:	  INUM	{ $$ = inum_node($1); set_nlist($$); }
	| RNUM	{ $$ = rnum_node($1); set_nlist($$); }
	| VAR	{ $$ = var_node($1); set_nlist($$); }
	| '(' poly ')'	{ $$ = $2; }
	;
%%

char *progname;
int lineno = 1;

static void test(void);

int main(int argc, char *argv[])
{
	progname = argv[0];
	/* test(); */
	ASTNode *nlist = NULL;
	yyparse(nlist);
	return 0;
}

int yyerror(ASTNode *nlist, const char *msg)
{
	fprintf(stderr, "%s: %s near line %d\n", progname, msg, lineno);
	free_node(nlist);
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
