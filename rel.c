#include "rel.h"
#include "term.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// Allocate and initialize a `RelNode`.
RelNode *rnode(Rel rel, TermNode *left, TermNode *right)
{
	RelNode *rnode = malloc(sizeof *rnode);
	*rnode = (RelNode){rel, left, right};
	return rnode;
}

// Normalize `r`.
bool norm_rel(RelNode *r)
{
	bool success = sub_poly(&r->left, r->right);
	r->right = icoeff_term(0);
	return success;
}

// Print an S-exp of the subtree under `r`.
void print_rel(const RelNode *r)
{
	static const char REL_SYM[][3] = {"=", ">", ">=", "<", "<="};
	print_poly(r->left);
	printf("%s ", REL_SYM[r->rel]);
	print_poly(r->right);
}

// Release `r` and all its child nodes.
void free_rel(RelNode *r)
{
	free_poly(r->left);
	free_poly(r->right);
	free(r);
}
