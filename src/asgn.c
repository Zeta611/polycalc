#include "asgn.h"
#include "term.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// Sets variable `name` to `poly` in `*env`.
bool set_var(char *name, struct TermNode *poly, EnvFrame **env)
{
	if (lookup(name, *env)) { // `name` is already defined in `*env`.
		return false;
	}
	EnvFrame *fr = malloc(sizeof *fr);
	*fr = (EnvFrame){name, poly, *env};
	*env = fr;
	return true;
}

// Returns a `TermNode *` assigned to `name` if it exists, `NULL` otherwise.
struct TermNode *lookup(const char *name, const EnvFrame *env)
{
	for (const EnvFrame *fr = env; fr; fr = fr->next) {
		if (strcmp(name, fr->name) == 0) {
			return fr->poly;
		}
	}
	return NULL;
}

// Release `env`.
void free_env(EnvFrame *env)
{
	if (!env) {
		return;
	}
	free(env->name);
	free_poly(env->poly);
	free_env(env->next);
	free(env);
}
