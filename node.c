#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "poly.h"

static void add_poly(TermNode **dest, TermNode *src);
static TermNode *coeff_term(double val);
static TermNode *var_term(char *name, int pow);
static void free_term(TermNode *t);

/******************************* AST handling ********************************/

ASTNode *op_node(enum Op op, ASTNode *left, ASTNode *right)
{
	ASTNode *node = malloc(sizeof *node);
	*node = (ASTNode){ OP_NODE, .u.dat = { op, left, right } };
	return node;
}

ASTNode *num_node(double val)
{
	ASTNode *node = malloc(sizeof *node);
	*node = (ASTNode){ NUM_NODE, .u.val = val };
	return node;
}

ASTNode *var_node(char *name)
{
	ASTNode *node = malloc(sizeof *node);
	*node = (ASTNode){ VAR_NODE, .u.name = name };
	return node;
}

void free_node(ASTNode *node)
{
	if (!node) {
		return;
	}
	switch (node->type) {
	case OP_NODE:
		free_node(node->u.dat.left);
		free_node(node->u.dat.right);
		break;
	case NUM_NODE:
		break;
	case VAR_NODE:
		free(node->u.name);
		break;
	default:
		fprintf(stderr, "unexpected node type %d\n", node->type);
		abort();
	}
	free(node);
}

void debug_node(const ASTNode *node)
{
	switch (node->type) {
	case OP_NODE:
		putchar('(');
		switch (node->u.dat.op) {
		case ADD:
			putchar('+');
			break;
		case MUL:
			putchar('*');
			break;
		default:
			fprintf(stderr, "unknown op type %d\n", node->u.dat.op);
			abort();
		}
		putchar(' ');
		debug_node(node->u.dat.left);
		putchar(' ');
		debug_node(node->u.dat.right);
		putchar(')');
		return;
	case NUM_NODE:
		printf("%lf", node->u.val);
		return;
	case VAR_NODE:
		printf("%s", node->u.name);
		return;
	default:
		fprintf(stderr, "unexpected node type %d\n", node->type);
		abort();
	}
}

TermNode *eval_node(const ASTNode *node)
{
	switch (node->type) {
	case OP_NODE: {
		enum Op op = node->u.dat.op;
		TermNode *lt = eval_node(node->u.dat.left);
		TermNode *rt = eval_node(node->u.dat.right);

		switch (op) {
		case ADD:
			add_poly(&lt, rt);
			return lt;
		/** case MUL: // TODO */
		/**         return lt; */
		default:
			fprintf(stderr, "unknown op type %d\n", op);
			abort();
		}
	}
	case NUM_NODE:
		return coeff_term(node->u.val);
	case VAR_NODE: {
		TermNode *ct = coeff_term(1);
		TermNode *vt = var_term(node->u.name, 1);
		ct->u.vars = vt;
		return ct;
	}
	default:
		fprintf(stderr, "unexpected node type %d\n", node->type);
		abort();
	}
}

/**************************** Polynomial handling ****************************/

static TermNode *coeff_term(double val)
{
	TermNode *term = malloc(sizeof *term);
	*term = (TermNode){ COEFF_TERM, .hd.val = val, .u.vars = NULL, NULL };
	return term;
}

static TermNode *var_term(char *name, int pow)
{
	TermNode *term = malloc(sizeof *term);
	char *s = malloc(strlen(name) + 1);
	strcpy(s, name);
	*term = (TermNode){ VAR_TERM, .hd.name = s, .u.pow = pow, NULL };
	return term;
}

// First prioritize reverse-lexicographically, then prioritize higher orders.
// t1 and t2 must be VARM_TERM nodes or NULLs.
static int var_cmp(const TermNode *t1, const TermNode *t2) {
	if (!t1 && !t2) {
		return 0;
	} else if (t1 && !t2) {
		return 1;
	} else if (!t1 && t2) {
		return -1;
	}
	// t1->type and t2->type must both be VAR_TERMs
	int cmp = strcmp(t1->hd.name, t2->hd.name);
	if (cmp) {
		return -cmp; // prioritize reverse-lexicographically
	}
	// t1 and t2 start with a same variable term
	cmp = t1->u.pow - t2->u.pow;
	if (cmp) {
		return cmp;
	}
	return var_cmp(t1->next, t2->next);
}

// Add src to dest.
//
// Argument passed to `src` must not be used after `add_poly` is called.
// `TermNode`s composing the polynomial represented by `src` are either rewired
// to `dest` accordingly or completely released from memory.
static void add_poly(TermNode **dest, TermNode *src)
{
	TermNode **p = dest;
	while (src) {
		if (!*p) {
			*p = src;
			return;
		}
		int cmp = var_cmp((*p)->u.vars, src->u.vars);
		if (cmp > 0) {
			p = &(*p)->next;
		} else if (cmp < 0) {
			TermNode *tmp = src->next;
			src->next = *p;
			*p = src;
			src = tmp;
		} else {
			(*p)->hd.val += src->hd.val;
			p = &(*p)->next;
			TermNode *tmp = src;
			src = src->next;
			free_term(tmp);
		}
	}
}

// Release a single `COEFF_TERM` or all linked `VAR_TERM`s.
// Does not recursively release `COEFF_TERM` terms. For that purpose, use `free_poly`.
static void free_term(TermNode *t)
{
	if (!t) {
		return;
	}
	switch (t->type) {
	case COEFF_TERM:
		free_term(t->u.vars);
		break;
	case VAR_TERM:
		free(t->hd.name);
		free_term(t->next);
		break;
	}
	free(t);
}

void print_poly(const TermNode *p) {
	while (p) {
		printf("%lf ", p->hd.val);
		for (TermNode *vt = p->u.vars; vt; vt = vt->next) {
			printf("%s^%d ", vt->hd.name, vt->u.pow);
		}
		p = p->next;
		if (p) {
			printf("+ ");
		}
	}
	putchar('\n');
}

// Release a polynomial, i.e., `COEFF_TERM` typed `TermNode` linked together.
void free_poly(TermNode *p)
{
	if (!p) {
		return;
	}
	free_poly(p->next);
	free_term(p);
}

void test(void)
{
	printf("TESTING\n");

	TermNode *p1 = coeff_term(2);
	p1->u.vars = var_term("x", 1);
	p1->u.vars->next = var_term("y", 2);
	p1->next = coeff_term(5);
	p1->next->u.vars = var_term("y", 1);
	p1->next->next = coeff_term(9);

	TermNode *p2 = coeff_term(1);
	p2->u.vars = var_term("x", 2);
	p2->next = coeff_term(3);
	p2->next->u.vars = var_term("x", 1);
	p2->next->u.vars->next = var_term("y", 2);
	p2->next->next = coeff_term(1);
	p2->next->next->u.vars = var_term("x", 1);
	p2->next->next->next = coeff_term(1);

	printf("P1: ");
	print_poly(p1);

	printf("P2: ");
	print_poly(p2);

	printf("P1 + P2: ");
	add_poly(&p1, p2); // p2 points to garbage henceforth

	print_poly(p1);
	free_poly(p1);
}
