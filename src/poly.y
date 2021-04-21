%code top {
#include "term.h"
#include <stdbool.h>
#include <stdio.h>
}

%code requires {
#include "asgn.h" // TODO
#include "ast.h"
#include "rel.h"
}

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
%token		ASGN

%type	<node>	atom expt neg mult poly rels asgn

%destructor { free($$); }	VAR
%destructor { free_node($$); }	<node>

%parse-param { EnvFrame **env }

%%

prgm:	  // nothing
	| prgm '\n'
	| prgm error '\n'
	| prgm rels '\n' {
		printf("AST: ");
		print_node($2);
		putchar('\n');
		RelNode *r;
		if ((r = eval_rel($2, *env))) {
			printf("REL: ");
			print_rel(r);
			putchar('\n');
			free_rel(r);
		}
		putchar('\n');
		free_node($2); }
	| prgm poly '\n' {
		printf("AST: ");
		print_node($2);
		putchar('\n');
		TermNode *p;
		if ((p = eval_poly($2, *env))) {
			printf("VAL: ");
			print_poly(p);
			putchar('\n');
			free_poly(p);
		}
		putchar('\n');
		free_node($2); }
	| prgm asgn '\n' {
		printf("AST: ");
		print_node($2);
		putchar('\n');
		const char *name = $2->u.asgndat.left->u.name;
		TermNode *p;
		if ((p = eval_asgn($2, env))) {
			printf("ASN: %s := ", name);
			print_poly(p);
			putchar('\n');
		} else {
			printf("Variable %s is already defined or "
			       "self-referenced.\n",
			       name);
		}
		putchar('\n');
		free_node($2); }
	;
asgn:	  VAR ASGN poly	{ $$ = asgn_node(var_node($1), $3); }
	;
rels:	  poly REL poly	{ $$ = rel_node($2, $1, $3); }
	| poly REL poly '&' rels {
		$$ = rel_node($2, $1, $3);
		$$->u.reldat.next = $5; }
	;
poly:	  mult
	| poly '+' mult	{ $$ = op_node(ADD, $1, $3); }
	| poly '-' mult	{ $$ = op_node(SUB, $1, $3); }
	;
mult:	  neg
	| mult '*' neg	{ $$ = op_node(MUL, $1, $3); }
	| mult '/' neg	{ $$ = op_node(DIV, $1, $3); }
	| mult expt	{ $$ = op_node(MUL, $1, $2); }
	;
neg:	  expt
	| '-' neg	{ $$ = op_node(NEG, $2, NULL); }
	;
expt:	  atom
	| atom '^' neg	{ $$ = op_node(POW, $1, $3); }
	;
atom:	  INUM	{ $$ = inum_node($1); }
	| RNUM	{ $$ = rnum_node($1); }
	| VAR	{ $$ = var_node($1); }
	| '(' poly ')'	{ $$ = $2; }
	;
%%

char *progname;
int lineno = 1;
extern FILE *yyin;

static void test(void);

int main(int argc, char *argv[])
{
	progname = argv[0];

	bool fin = false;
	if (argc > 1) {
		fin = true;
		yyin = fopen(argv[1], "r");
	}
	/* test(); */
	EnvFrame *env = NULL;
	yyparse(&env);
	free_env(env);

	if (fin) {
		fclose(yyin);
	}
	return 0;
}

int yyerror(EnvFrame **env, const char *msg)
{
	(void)env;
	fprintf(stderr, "%s: %s near line %d\n", progname, msg, lineno);
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
