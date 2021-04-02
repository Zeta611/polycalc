#ifndef AST_H
#define AST_H

// `ASTNode` is the building block of our AST.
typedef struct ASTNode {
	enum { REL_NODE, OP_NODE, INUM_NODE, RNUM_NODE, VAR_NODE } type;
	union {
		struct {
			enum Rel { EQ = 1, GT, GE, LT, LE } rel;
			struct ASTNode *left, *right, *next;
		} reldat; // REL_NODE
		struct {
			enum Op { ADD, SUB, MUL, DIV, POW, NEG } op;
			struct ASTNode *left, *right;
		} opdat;     // OP_NODE
		long ival;   // INUM_NODE
		double rval; // RNUM_NODE
		char *name;  // VAR_NODE
	} u;
	struct ASTNode *next;
} ASTNode;

typedef enum Rel Rel;
typedef enum Op Op;

// Allocate and initialize a `REL_NODE` type node.
ASTNode *rel_node(Rel rel, ASTNode *left, ASTNode *right);

// Allocate and initialize an `OP_NODE` type node.
ASTNode *op_node(Op op, ASTNode *left, ASTNode *right);

// Allocate and initialize a `INUM_NODE` type node.
ASTNode *inum_node(long val);

// Allocate and initialize a `RNUM_NODE` type node.
ASTNode *rnum_node(double val);

// Allocate and initialize a `VAR_NODE` type node.
ASTNode *var_node(char *name);

// Release `node` and all its child nodes.
void free_node(ASTNode *node);

// Print an S-exp of the subtree under `node`.
void print_node(const ASTNode *node);

struct TermNode;
// Return the resulting polynomial evaluating the subtree under `node`.
struct TermNode *eval_poly(const ASTNode *node);

struct RelNode;
// Return the resulting relation evaluating the subtree under `node`.
struct RelNode *eval_rel(const ASTNode *node);

#endif /* ifndef AST_H */
