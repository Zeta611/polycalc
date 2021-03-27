#include "ast.h"
#include "term.h"
#include <stdio.h>
#include <stdlib.h>

ASTNode *op_node(enum Op op, ASTNode *left, ASTNode *right)
{
	ASTNode *node = malloc(sizeof *node);
	*node = (ASTNode){OP_NODE, .u.dat = {op, left, right}};
	return node;
}

ASTNode *inum_node(long val)
{
	ASTNode *node = malloc(sizeof *node);
	*node = (ASTNode){INUM_NODE, .u.ival = val};
	return node;
}

ASTNode *rnum_node(double val)
{
	ASTNode *node = malloc(sizeof *node);
	*node = (ASTNode){RNUM_NODE, .u.rval = val};
	return node;
}

ASTNode *var_node(char *name)
{
	ASTNode *node = malloc(sizeof *node);
	*node = (ASTNode){VAR_NODE, .u.name = name};
	return node;
}

void free_node(ASTNode *node)
{
	if (!node) {
		return; // Right child of a `NEG` node is `NULL`.
	}
	switch (node->type) {
	case OP_NODE:
		free_node(node->u.dat.left);
		free_node(node->u.dat.right);
		break;
	case INUM_NODE:
	case RNUM_NODE:
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
	static const char OP_SYM[] = {'+', '-', '*', '/', '^', '-'};
	switch (node->type) {
	case OP_NODE:
		putchar('(');
		putchar(OP_SYM[node->u.dat.op]);
		putchar(' ');
		print_node(node->u.dat.left);
		if (node->u.dat.op != NEG) {
			putchar(' ');
			print_node(node->u.dat.right);
		}
		putchar(')');
		return;
	case INUM_NODE:
		printf("%ld", node->u.ival);
		return;
	case RNUM_NODE:
		printf("%lf", node->u.rval);
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
	if (!node) { // for NEG op
		return NULL;
	}
	switch (node->type) {
	case OP_NODE: {
		enum Op op = node->u.dat.op;
		TermNode *lt = eval_node(node->u.dat.left);
		TermNode *rt = eval_node(node->u.dat.right);

		// Result of `eval_node` being `NULL` indicates an invalid
		// syntax or an operation, except for the result of evaluating
		// the right child of the `NEG` op.
		if (!lt || (!rt && op != NEG)) {
			free_poly(lt);
			free_poly(rt);
			return NULL;
		}

		bool success;
		switch (op) {
		case ADD:
			success = add_poly(&lt, rt);
			break;
		case SUB:
			success = sub_poly(&lt, rt);
			break;
		case MUL:
			success = mul_poly(&lt, rt);
			break;
		case DIV:
			success = div_poly(&lt, rt);
			break;
		case POW:
			success = pow_poly(&lt, rt);
			break;
		case NEG:
			success = neg_poly(lt);
			break;
		default:
			fprintf(stderr, "unknown op type %d\n", op);
			abort();
		}
		if (!success) {
			free_poly(lt);
			return NULL;
		}
		return lt;
	}
	case INUM_NODE:
		return icoeff_term(node->u.ival);
	case RNUM_NODE:
		return rcoeff_term(node->u.rval);
	case VAR_NODE: {
		TermNode *ct = icoeff_term(1);
		TermNode *vt = var_term(node->u.name, 1);
		ct->u.vars = vt;
		return ct;
	}
	default:
		fprintf(stderr, "unexpected node type %d\n", node->type);
		abort();
	}
}
