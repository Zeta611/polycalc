#ifndef REL_H
#define REL_H

#include "ast.h"
#include <stdbool.h>

typedef struct RelNode {
	Rel rel;
	struct TermNode *left;
	struct TermNode *right;
	struct RelNode *next;
} RelNode;

struct TermNode;
// Allocate and initialize a `RelNode`.
RelNode *rnode(Rel rel, struct TermNode *left, struct TermNode *right);

Rel merge_rel(Rel r1, Rel r2);

// Normalize `r`.
bool norm_rel(RelNode *r);

// Check `n Rel 0` type of relations
bool verify_nrel(const RelNode *r);

// Print an S-exp of the subtree under `r`.
void print_rel(const RelNode *r);

// Release `r` and all its child nodes.
void free_rel(RelNode *r);

#endif /* ifndef REL_H */
