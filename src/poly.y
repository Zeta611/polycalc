%code top {
#include "term.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
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

%parse-param { EnvFrame **env } { bool *verbose }

%%

prgm:	  // nothing
	| prgm '\n'
	| prgm error '\n'
	| prgm rels '\n' {
		if (*verbose) {
			printf("AST: ");
			print_node($2);
			putchar('\n');
		}
		RelNode *r;
		if ((r = eval_rel($2, *env))) {
			if (*verbose) {
				printf("REL: ");
			}
			print_rel(r);
			putchar('\n');
			free_rel(r);
		}
		if (*verbose) {
			putchar('\n');
		}
		free_node($2); }
	| prgm poly '\n' {
		if (*verbose) {
			printf("AST: ");
			print_node($2);
			putchar('\n');
		}
		TermNode *p;
		if ((p = eval_poly($2, *env))) {
			if (*verbose) {
				printf("VAL: ");
			}
			print_poly(p);
			putchar('\n');
			free_poly(p);
		}
		if (*verbose) {
			putchar('\n');
		}
		free_node($2); }
	| prgm asgn '\n' {
		if (*verbose) {
			printf("AST: ");
			print_node($2);
			putchar('\n');
		}
		const char *name = $2->u.asgndat.left->u.name;
		TermNode *p;
		if ((p = eval_asgn($2, env))) {
			if (*verbose) {
				printf("ASN: %s := ", name);
			}
			print_poly(p);
			putchar('\n');
		} else {
			fprintf(stderr, "Variable %s is already defined or "
			       "self-referenced.\n",
			       name);
		}
		if (*verbose) {
			putchar('\n');
		}
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

int main(int argc, char *argv[])
{
	progname = argv[0];

	// Parse command line arguments.
	bool verbose = true;
	bool fin = false;
	int optidx;
	for (optidx = 1; optidx < argc && argv[optidx][0] == '-'; ++optidx) {
		switch (argv[optidx][1]) {
		case 'q':
			verbose = false;
			break;
		case 'v':
			break;
		default:
			fprintf(stderr, "Usage: %s [-qv] [file] \n", progname);
			exit(EXIT_FAILURE);
		}
	}
	argv += optidx; // `argv` points to the remaining non-option arguments.
	if (*argv) {
		fin = true;
		yyin = fopen(*argv, "r");
	}

	EnvFrame *env = NULL;
	yyparse(&env, &verbose);
	free_env(env);

	if (fin) {
		fclose(yyin);
	}
	return 0;
}

int yyerror(EnvFrame **env, bool *verbose, const char *msg)
{
	(void)env;
	(void)verbose;
	fprintf(stderr, "%s: %s near line %d\n", progname, msg, lineno);
	return 0;
}
