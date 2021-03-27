#ifndef REL_H
#define REL_H

#include "ast.h"

typedef struct RelNode {
	Rel rel;
	struct TermNode *left;
	struct TermNode *right;
} RelNode;

void print_rel(const RelNode *r);
void free_rel(RelNode *r);

#endif /* ifndef REL_H */
