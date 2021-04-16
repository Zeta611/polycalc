#ifndef ASGN_H
#define ASGN_H
#include <stdbool.h>

struct TermNode;
typedef struct EnvFrame {
	char *name;
	struct TermNode *poly;
	struct EnvFrame *next;
} EnvFrame;

// Sets variable `name` to `poly` in `*env`.
bool set_var(char *name, struct TermNode *poly, EnvFrame **env);

// Returns a `TermNode *` assigned to `name` if it exists, `NULL` otherwise.
struct TermNode *lookup(const char *name, const EnvFrame *env);

// Release `env`.
void free_env(EnvFrame *env);

#endif /* ifndef ASGN_H */
