#include "rel.h"
#include "term.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// Allocate and initialize a `RelNode`.
RelNode *rnode(Rel rel, TermNode *left, TermNode *right)
{
	RelNode *rnode = malloc(sizeof *rnode);
	*rnode = (RelNode){rel, left, right, NULL};
	return rnode;
}

// Normalize `r`.
bool norm_rel(RelNode *r)
{
	bool success = sub_poly(&r->left, r->right);
	r->right = icoeff_term(0);
	return success;
}

// Print S-exps of the subtrees under `r` and linked nodes.
void print_rel(const RelNode *r)
{
	static const char REL_SYM[][3] = {"=", ">", ">=", "<", "<="};
	print_poly(r->left);
	printf("%s ", REL_SYM[r->rel]);
	print_poly(r->right);
	if (r->next) {
		printf(" &\n");
		print_rel(r->next);
	}
}

// Release `r` and all of the linked nodes along with their child nodes.
void free_rel(RelNode *r)
{
	if (!r) {
		return;
	}
	free_poly(r->left);
	free_poly(r->right);
	free_rel(r->next);
	free(r);
}
