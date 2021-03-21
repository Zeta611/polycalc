#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "poly.h"

// Forward declarations for static functions
static TermNode *coeff_term(double val);
static TermNode *var_term(char *name, int pow);

static int var_cmp(const TermNode *t1, const TermNode *t2);
static void add_poly(TermNode **dest, TermNode *src);

static TermNode *term_dup(const TermNode *t);
static TermNode *var_dup(const TermNode *v);
static TermNode *poly_dup(const TermNode *p);
static void mul_var(TermNode **dest, TermNode *src);
static void mul_poly(TermNode **dest, TermNode *src);

static void free_term(TermNode *t);
static void print_var(const TermNode *v);

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
		return; // Simply here for defense--should never be reached.
	}
	switch (node->type) {
	case OP_NODE:
		free_node(node->u.dat.left);
		free_node(node->u.dat.right);
		break;
	case NUM_NODE:
		break;
	case VAR_NODE:
		free(node->u.name); // `u.name` was allocated in the lexer.
		break;
	default:
		fprintf(stderr, "unexpected node type %d\n", node->type);
		abort();
	}
	free(node);
}

void print_node(const ASTNode *node)
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
		print_node(node->u.dat.left);
		putchar(' ');
		print_node(node->u.dat.right);
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
		case MUL:
			mul_poly(&lt, rt);
			return lt;
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
// `t1` and `t2` must be `VARM_TERM`s or `NULL`s.
static int var_cmp(const TermNode *t1, const TermNode *t2)
{
	if (!t1 && !t2) {
		return 0;
	} else if (t1 && !t2) {
		return 1;
	} else if (!t1 && t2) {
		return -1;
	}
	// `t1->type` and `t2->type` must both be `VAR_TERM`s.
	int cmp = strcmp(t1->hd.name, t2->hd.name);
	if (cmp) {
		// Prioritize reverse-lexicographically, e.g., 'x' > 'y' > 'z'.
		return -cmp;
	}
	// `t1` and `t2` start with a same variable term.
	cmp = t1->u.pow - t2->u.pow;
	if (cmp) {
		return cmp;
	}
	return var_cmp(t1->next, t2->next);
}

// Add `src` to `dest`.
// This is essentially merging two linked lists.
// Argument passed to `src` must not be used after `add_poly` is called.
// `TermNode`s composing the polynomial represented by `src` are either rewired
// to `dest` accordingly or completely released from memory.
static void add_poly(TermNode **dest, TermNode *src)
{
	// `*p` is the head pointer initially; `next` of a `TermNode`, if
	// traversed.
	// The double-pointer allows a uniform handling of both the head
	// pointer and the `next` pointer, by storing an address of a pointer.
	TermNode **p = dest;
	while (src) {
		if (!*p) {
			// Link rest of `src` at the end.
			*p = src;
			return;
		}
		int cmp = var_cmp((*p)->u.vars, src->u.vars);
		if (cmp > 0) {
			p = &(*p)->next;
		} else if (cmp < 0) {
			// Rewire `*src` to the `**p`-side.
			TermNode *tmp = src->next;
			src->next = *p;
			*p = src;
			src = tmp;
		} else {
			// Increase the coefficient of `**p` and release `src`.
			(*p)->hd.val += src->hd.val;
			p = &(*p)->next;
			TermNode *tmp = src;
			src = src->next;
			free_term(tmp);
		}
	}
}

static TermNode *term_dup(const TermNode *t)
{
	switch (t->type) {
	case COEFF_TERM:
		return coeff_term(t->hd.val);
	case VAR_TERM:
		return var_term(t->hd.name, t->u.pow);
	default:
		fprintf(stderr, "unexpected node type %d\n", t->type);
		abort();
	}
}

static TermNode *var_dup(const TermNode *v)
{
	TermNode *ret, **dup;
	dup = NULL;
	for (ret = term_dup(v); v; v = v->next, dup = &(*dup)->next) {
		if (!dup) {
			dup = &ret;
		} else {
			*dup = term_dup(v);
		}
	}
	return ret;
}

static TermNode *poly_dup(const TermNode *p)
{
	TermNode *ret, **dup;
	dup = NULL;
	for (ret = term_dup(p); p; p = p->next, dup = &(*dup)->next) {
		if (!dup) {
			dup = &ret;
		} else {
			*dup = term_dup(p);
		}
		TermNode **vdup = &(*dup)->u.vars;
		for (TermNode *vt = p->u.vars; vt; vt = vt->next) {
			*vdup = term_dup(vt);
			vdup = &(*vdup)->next;
		}
	}
	return ret;
}

// Multiply `src` to `dest`--both should point directly to `VAR_TERM`s.
// This is analagous to `add_poly` function, as it is merging two sorted
// `VAR_TERM` lists.
static void mul_var(TermNode **dest, TermNode *src)
{
	TermNode **p = dest;
	while (src) {
		if (!*p) {
			*p = src;
			return;
		}
		// Again, prioritize reverse-lexicographically.
		int cmp = -strcmp((*p)->hd.name, src->hd.name);
		if (cmp > 0) {
			p = &(*p)->next;
		} else if (cmp < 0) {
			TermNode *tmp = src->next;
			src->next = *p;
			*p = src;
			src = tmp;
		} else {
			(*p)->u.pow += src->u.pow;
			p = &(*p)->next;
			TermNode *tmp = src;
			src = src->next;
			// No dedicated function to free a single `VAR_TERM`.
			free(tmp->hd.name);
			free(tmp);
		}
	}
}

// Multiply `src` to `dest`.
// Uses distributive law to multiply.
// Argument passed to `src` must not be used after `mul_poly` is called.
static void mul_poly(TermNode **dest, TermNode *src)
{
	// `dup` points to the head pointer initially, and then points to the
	// copy of `*dest` afterwords (only if there are more than one terms
	// in `src` to apply distributive law).
	// `dup` is there only to keep a pointer later to be assigned to `p`.
	TermNode **dup, **p;
	for (dup = p = dest; src; p = dup) {
		if (src->next) {
			TermNode *tmp = poly_dup(*dup);
			// Get a fresh slot for a head pointer container.
			dup = malloc(sizeof *dup);
			*dup = tmp;
		}
		TermNode *svars = src->u.vars;
		// Multiply a term `*src` to each of the terms in `*p`.
		for (TermNode **tmp = p; *tmp; tmp = &(*tmp)->next) {
			(*tmp)->hd.val *= src->hd.val;
			if (svars) {
				mul_var(&(*tmp)->u.vars, var_dup(svars));
			}
		}
		if (p != dest) {
			add_poly(dest, *p);
			free(p); // Allocated by `dup`.
		}
		TermNode *tmp = src;
		src = src->next;
		free_term(tmp);
	}
}

// Release a single `COEFF_TERM` and/or all linked `VAR_TERM`s.
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
	default:
		fprintf(stderr, "unexpected node type %d\n", t->type);
		abort();
	}
	free(t);
}

static void print_var(const TermNode *v)
{
	for (; v; v = v->next) {
		printf("%s^%d ", v->hd.name, v->u.pow);
	}
}

void print_poly(const TermNode *p)
{
	while (p) {
		printf("%lf ", p->hd.val);
		print_var(p->u.vars);
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

	// 2xy^2 + 5y + 9
	TermNode *p1 = coeff_term(2);
	p1->u.vars = var_term("x", 1);
	p1->u.vars->next = var_term("y", 2);
	p1->next = coeff_term(5);
	p1->next->u.vars = var_term("y", 1);
	p1->next->next = coeff_term(9);

	// x^2 + 3xy^2 + x + 1
	TermNode *p2 = coeff_term(1);
	p2->u.vars = var_term("x", 2);
	p2->next = coeff_term(3);
	p2->next->u.vars = var_term("x", 1);
	p2->next->u.vars->next = var_term("y", 2);
	p2->next->next = coeff_term(1);
	p2->next->next->u.vars = var_term("x", 1);
	p2->next->next->next = coeff_term(1);

	printf("P1: \n");
	print_poly(p1);

	printf("P2: \n");
	print_poly(p2);

	printf("P1 * P2: \n");
	mul_poly(&p1, p2); // `p2` points to garbage henceforth.
	print_poly(p1);
	free_poly(p1);
}
