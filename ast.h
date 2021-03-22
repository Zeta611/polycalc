#ifndef AST_H
#define AST_H

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

struct TermNode;
// Return the resulting polynomial evaluating the subtree under `node`
struct TermNode *eval_node(const ASTNode *node);

#endif /* ifndef AST_H */
