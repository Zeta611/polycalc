#include "ast.h"
#include "asgn.h"
#include "rel.h"
#include "term.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Allocate and initialize a `ASGN_NODE` type node.
ASTNode *asgn_node(ASTNode *left, ASTNode *right)
{
	ASTNode *node = malloc(sizeof *node);
	*node = (ASTNode){ASGN_NODE, .u.asgndat = {left, right}};
	return node;
}

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
		return;
	}
	switch (node->type) {
	case ASGN_NODE:
		free_node(node->u.asgndat.left);
		free_node(node->u.asgndat.right);
		break;
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
		free(node->u.name);
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
	static const char REL_SYM[][3] = {"", "=", ">", ">=", "<", "<="};

	switch (node->type) {
	case ASGN_NODE:
		printf("(:= ");
		print_node(node->u.asgndat.left);
		putchar(' ');
		print_node(node->u.asgndat.right);
		putchar(')');
		return;
	case REL_NODE:
		printf("(%s ", REL_SYM[node->u.reldat.rel]);
		print_node(node->u.reldat.left);
		putchar(' ');
		print_node(node->u.reldat.right);
		putchar(')');
		if (node->u.reldat.next) {
			printf(" & ");
			print_node(node->u.reldat.next);
		}
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
TermNode *eval_poly(const ASTNode *node, const EnvFrame *env)
{
	if (!node) { // for NEG op
		return NULL;
	}
	switch (node->type) {
	case OP_NODE: {
		Op op = node->u.opdat.op;
		TermNode *lt = eval_poly(node->u.opdat.left, env);
		TermNode *rt = eval_poly(node->u.opdat.right, env);

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
		TermNode *p;
		// Check if there is already a term assigned to `node->u.name`
		// in `env`.
		if ((p = lookup(node->u.name, env))) {
			return poly_dup(p);
		} else {
			p = icoeff_term(1);
			TermNode *vt = var_term(node->u.name, 1);
			p->u.vars = vt;
			return p;
		}
	}
	default:
		fprintf(stderr, "unexpected node type %d\n", node->type);
		abort();
	}
}

// Return the resulting relation evaluating the subtree under `node`.
RelNode *eval_rel(const ASTNode *node, const EnvFrame *env)
{
	TermNode *left = eval_poly(node->u.reldat.left, env);
	TermNode *right = eval_poly(node->u.reldat.right, env);
	if (!left || !right) { // Exception while evaluating `left` or `right`.
		free_poly(left);
		free_poly(right);
		return NULL;
	}

	// Both `left` and `right` are now owned by `r`.
	RelNode *r = rnode(node->u.reldat.rel, left, right);
	RelNode *hd = NULL; // For rest of the relations in the system.
	if (norm_rel(r)) {
		if (!r->left->u.vars && !verify_nrel(r)) {
			goto inconsistent_sys;
		}
		if (node->u.reldat.next) {
			hd = eval_rel(node->u.reldat.next, env);
			if (!hd) { // Exception in previous relations.
				goto r_cleanup;
			}
			if (!hd->rel) {
				goto inconsistent_sys;
			}
			int c;
			RelNode **p = &hd;
			while (*p && (c = poly_cmp(r->left, (*p)->left)) < 0) {
				p = &(*p)->next;
			}
			if (*p && !c) { // Same polynomial found.
				Rel rel = merge_rel(r->rel, (*p)->rel);
				if (rel) {
					free_rel(r);
					(*p)->rel = rel;
					return hd;
				} else {
					goto inconsistent_sys;
				}
			} else {
				r->next = *p;
				*p = r;
				return hd;
			}
		} else {
			return r;
		}
	}
r_cleanup:
	free_rel(r);
	return NULL;
inconsistent_sys:
	free_rel(hd);
	free_poly(r->left);
	free_poly(r->right);
	r->left = r->right = NULL;
	r->rel = 0;
	return r;
}

// Return the assigned polynomial. `NULL` indicates a duplicate definition or a
// self-reference.
TermNode *eval_asgn(const ASTNode *node, EnvFrame **env)
{
	// Setup the variable name (LHS)
	const char *name = node->u.asgndat.left->u.name;
	char *s = malloc(strlen(name) + 1);
	strcpy(s, name);

	TermNode *poly = eval_poly(node->u.asgndat.right, *env);
	// Search for a cyclic definition.
	for (TermNode *t = poly; t; t = t->next) {
		for (TermNode *var = t->u.vars; var; var = var->next) {
			// Variables are sorted reverse-lexicographically.
			int cmp = -strcmp(var->hd.name, s);
			if (cmp == 0) { // A self-reference is found.
				goto cleanup;
			} else if (cmp < 0) {
				break;
			}
		}
	}

	if (!set_var(s, poly, env)) { // A variable named `name` already exists.
		goto cleanup;
	}
	return poly;
cleanup:
	free_poly(poly);
	free(s);
	return NULL;
}
