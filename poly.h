#ifndef POLY_H
#define POLY_H

// `ASTNode` is the building block of our AST
typedef struct ASTNode {
	enum { OP_NODE, NUM_NODE, VAR_NODE } type;
	union {
		struct {
			enum Op { ADD, MUL } op;
			struct ASTNode *left, *right;
		} dat; // OP_NODE
		double val; // NUM_NODE
		char *name; // VAR_NODE
	} u;
} ASTNode;

/* Diagram of the representation for 2xy^2 + 5y + 9 using `TermNode`s
 *
 *  hd   u  next
 * +---+---+---+   +---+---+---+   +---+---+---+
 * | 2 | # | #-+-->| 5 | # | #-+-->| 9 | $ | $ |
 * +---+-|-+---+   +---+-|-+---+   +---+---+---+
 *       |               v
 *       |             +---+---+---+
 *       |             | y | 1 | $ |
 *       |             +---+---+---+
 *       v
 *     +---+---+---+   +---+---+---+
 *     | x | 1 | #-+-->| y | 2 | $ |
 *     +---+---+---+   +---+---+---+
 */
typedef struct TermNode {
	enum { COEFF_TERM, VAR_TERM } type;
	union {
		double val; // COEFF_TERM
		char *name; // VAR_TERM
	} hd;
	union {
		struct TermNode *vars; // COEFF_TERM
		int pow; // VAR_TERM
	} u;
	struct TermNode *next;
} TermNode;

// Allocate and initialize an `OP_NODE` type node
ASTNode *op_node(enum Op op, ASTNode *left, ASTNode *right);

// Allocate and initialize a `NUM_NODE` type node
ASTNode *num_node(double val);

// Allocate and initialize a `VAR_NODE` type node
ASTNode *var_node(char *name);

// Release `node` and all its child nodes
void free_node(ASTNode *node);

// Print an S-exp of the subtree under `node`
void print_node(const ASTNode *node);

// Return the resulting polynomial evaluating the subtree under `node`
TermNode *eval_node(const ASTNode *node);

// Print a polynomial pointed by `p`.
void print_poly(const TermNode *p);

// Release a polynomial, i.e., `COEFF_TERM` typed `TermNode` linked together.
void free_poly(TermNode *p);

void test(void);

extern char *yytext;
extern int lineno;

#endif /* ifndef POLY_H */
