#include "rel.h"
#include "term.h"
#include <stdio.h>
#include <stdlib.h>

void print_rel(const RelNode *r)
{
	static const char REL_SYM[][3] = {"=", ">", ">=", "<", "<="};
	print_poly(r->left);
	printf("%s ", REL_SYM[r->rel]);
	print_poly(r->right);
}

void free_rel(RelNode *r)
{
	free_poly(r->left);
	free_poly(r->right);
	free(r);
}
