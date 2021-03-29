#include "ast.h"
#include "rel.h"
#include "term.h"
#include <stdio.h>
#include <stdlib.h>

// Allocate and initialize a `REL_NODE` type node.
ASTNode *rel_node(Rel rel, ASTNode *left, ASTNode *right)
{
	ASTNode *node = malloc(sizeof *node);
	*node = (ASTNode){REL_NODE, .u.reldat = {rel, left, right}};
	return node;
}

// Allocate and initialize an `OP_NODE` type node.
ASTNode *op_node(Op op, ASTNode *left, ASTNode *right)
{
	ASTNode *node = malloc(sizeof *node);
	*node = (ASTNode){OP_NODE, .u.opdat = {op, left, right}};
	return node;
}

// Allocate and initialize a `INUM_NODE` type node.
ASTNode *inum_node(long val)
{
	ASTNode *node = malloc(sizeof *node);
	*node = (ASTNode){INUM_NODE, .u.ival = val};
	return node;
}

// Allocate and initialize a `RNUM_NODE` type node.
ASTNode *rnum_node(double val)
{
	ASTNode *node = malloc(sizeof *node);
	*node = (ASTNode){RNUM_NODE, .u.rval = val};
	return node;
}

// Allocate and initialize a `VAR_NODE` type node.
ASTNode *var_node(char *name)
{
	ASTNode *node = malloc(sizeof *node);
	*node = (ASTNode){VAR_NODE, .u.name = name};
	return node;
}

// Release `node` and all its child nodes.
void free_node(ASTNode *node)
{
	if (!node) {
		return; // Right child of a `NEG` node is `NULL`.
	}
	switch (node->type) {
	case REL_NODE:
		free_node(node->u.reldat.left);
		free_node(node->u.reldat.right);
		free_node(node->u.reldat.next);
		break;
	case OP_NODE:
		free_node(node->u.opdat.left);
		free_node(node->u.opdat.right);
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

// Print an S-exp of the subtree under `node`.
void print_node(const ASTNode *node)
{
	static const char OP_SYM[] = {'+', '-', '*', '/', '^', '-'};
	static const char REL_SYM[][3] = {"=", ">", ">=", "<", "<="};

	switch (node->type) {
	case REL_NODE:
		printf("(%s ", REL_SYM[node->u.reldat.rel]);
		print_node(node->u.opdat.left);
		putchar(' ');
		print_node(node->u.opdat.right);
		putchar(')');
		return;
	case OP_NODE:
		printf("(%c ", OP_SYM[node->u.opdat.op]);
		print_node(node->u.opdat.left);
		if (node->u.opdat.op != NEG) {
			putchar(' ');
			print_node(node->u.opdat.right);
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

// Return the resulting polynomial evaluating the subtree under `node`.
TermNode *eval_poly(const ASTNode *node)
{
	if (!node) { // for NEG op
		return NULL;
	}
	switch (node->type) {
	case OP_NODE: {
		Op op = node->u.opdat.op;
		TermNode *lt = eval_poly(node->u.opdat.left);
		TermNode *rt = eval_poly(node->u.opdat.right);

		// Result of `eval_poly` being `NULL` indicates an invalid
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

// Return the resulting relation evaluating the subtree under `node`.
RelNode *eval_rel(const ASTNode *node)
{
	TermNode *left = eval_poly(node->u.reldat.left);
	TermNode *right = eval_poly(node->u.reldat.right);
	if (!left || !right) { // Exception while evaluating `left` or `right`.
		free_poly(left);
		free_poly(right);
		return NULL;
	}

	RelNode *r = rnode(node->u.reldat.rel, left, right);
	if (norm_rel(r)) {
		if (node->u.reldat.next) {
			RelNode *next = eval_rel(node->u.reldat.next);
			if (!next) {
				free_rel(r);
				return NULL;
			}
			r->next = next;
		}
		return r;
	} else {
		free_rel(r);
		return NULL;
	}
}
