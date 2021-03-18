#include <stdio.h>
#include <stdlib.h>
#include "poly.h"

Node *op_node(enum Op op, Node *left, Node *right)
{
	Node *node = malloc(sizeof *node);
	*node = (Node){ OP_NODE, .u.dat = { op, left, right } };
	return node;
}

Node *num_node(double num)
{
	Node *node = malloc(sizeof *node);
	*node = (Node){ NUM_NODE, .u.num = num };
	return node;
}

double eval_node(Node *node)
{
	switch (node->type) {
	case OP_NODE: {
		enum Op op = node->u.dat.op;
		double lval = eval_node(node->u.dat.left);
		double rval = eval_node(node->u.dat.right);

		switch (op) {
		case ADD:
			return lval + rval;
		case MUL:
			return lval * rval;
		default:
			fprintf(stderr, "unknown op type %d\n", op);
			abort();
		}
	}
	case NUM_NODE:
		return node->u.num;
	default:
		fprintf(stderr, "unexpected node type %d\n", node->type);
		abort();
	}
}

void free_node(Node *node)
{
	switch (node->type) {
	case OP_NODE:
		free_node(node->u.dat.left);
		free_node(node->u.dat.right);
		// fall through
	case NUM_NODE:
		free(node);
		return;
	default:
		fprintf(stderr, "unexpected node type %d\n", node->type);
		abort();
	}
}

void debug_node(Node *node)
{
	switch (node->type) {
	case OP_NODE:
		putchar('(');
		switch (node->u.dat.op) {
		case ADD:
			putchar('+');
			break;
		case MUL:
			putchar('*');
			break;
		default:
			fprintf(stderr, "unknown op type %d\n", node->u.dat.op);
			abort();
		}
		putchar(' ');
		debug_node(node->u.dat.left);
		putchar(' ');
		debug_node(node->u.dat.right);
		putchar(')');
		return;
	case NUM_NODE:
		printf("%lf", node->u.num);
		return;
	default:
		fprintf(stderr, "unexpected node type %d\n", node->type);
		abort();
	}
}
