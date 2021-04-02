#include "rel.h"
#include "term.h"
#include "util.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static Rel rev_rel(Rel rel)
{
	switch (rel) {
	case EQ:
		return EQ;
	case GT:
		return LT;
	case GE:
		return LE;
	case LT:
		return GT;
	case LE:
		return GE;
	default:
		return 0;
	}
}

// Allocate and initialize a `RelNode`.
RelNode *rnode(Rel rel, TermNode *left, TermNode *right)
{
	RelNode *rnode = malloc(sizeof *rnode);
	*rnode = (RelNode){rel, left, right, NULL};
	return rnode;
}

inline Rel merge_rel(Rel r1, Rel r2) { return r1 & r2; }

// Normalize `r`.
bool norm_rel(RelNode *r)
{
	bool success = sub_poly(&r->left, r->right);
	r->right = icoeff_term(0);
	if (!success) {
		return false;
	}

	long g = 0;
	for (TermNode *t = r->left; t; t = t->next) {
		if (t->type != ICOEFF_TERM) {
			continue;
		}
		if (!g) { // first ICOEFF_TERM
			g = labs(t->hd.ival);
		} else {
			g = gcd(labs(t->hd.ival), g);
		}
	}

	if (g > 1) {
		if (r->left->hd.ival < 0) {
			g = -g;
			r->rel = rev_rel(r->rel);
		}
		for (TermNode *t = r->left; t; t = t->next) {
			switch (t->type) {
			case ICOEFF_TERM:
				t->hd.ival /= g;
				break;
			case RCOEFF_TERM:
				t->hd.rval /= g;
				break;
			default:
				fprintf(stderr, "unexpected node type %d\n",
					t->type);
				abort();
			}
		}
	} else if (r->left->hd.ival < 0) {
		r->rel = rev_rel(r->rel);

		for (TermNode *t = r->left; t; t = t->next) {
			switch (t->type) {
			case ICOEFF_TERM:
				t->hd.ival *= -1;
				break;
			case RCOEFF_TERM:
				t->hd.rval *= -1.0;
				break;
			default:
				fprintf(stderr, "unexpected node type %d\n",
					t->type);
				abort();
			}
		}
	}
	return true;
}

bool verify_nrel(const RelNode *r)
{
	switch (r->rel) {
	case EQ:
		return coeff_cmp(r->left, r->right) == 0;
	case GT:
		return coeff_cmp(r->left, r->right) > 0;
	case GE:
		return coeff_cmp(r->left, r->right) >= 0;
	case LT:
		return coeff_cmp(r->left, r->right) < 0;
	case LE:
		return coeff_cmp(r->left, r->right) <= 0;
	default:
		return false;
	}
}

// Print S-exps of the subtrees under `r` and linked nodes.
void print_rel(const RelNode *r)
{
	static const char REL_SYM[][3] = {"", "=", ">", ">=", "<", "<="};
	if (r->rel) {
		print_poly(r->left);
		printf("%s ", REL_SYM[r->rel]);
		print_poly(r->right);
		if (r->next) {
			printf("\n   & ");
			print_rel(r->next);
		}
	} else {
		printf("INCONSISTENT SYSTEM");
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
