#ifndef POLY_H
#define POLY_H

typedef struct Node {
	enum { OP_NODE, NUM_NODE } type;
	union {
		struct {
			enum Op { ADD, MUL } op;
			struct Node *left, *right;
		} dat; // OP_NODE
		double num; // NUM_NODE
	} u;
} Node;


Node *op_node(enum Op op, Node *left, Node *right);
Node *num_node(double num);

double eval_node(Node *node);
void free_node(Node *node);

void debug_node(Node *node);

extern char *yytext;
extern int lineno;

#endif /* ifndef POLY_H */
