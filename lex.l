%{
#include <stdio.h>
#include <string.h>
#include "y.tab.h"

extern char *yytext;
extern int lineno;
%}

ws	[ \t]+

ulph	[_a-zA-Z]
ulnum	[_a-zA-Z0-9]
var	{ulph}|'{ulph}{ulnum}*

digit	[0-9]
int	{digit}+
pow	[eE][+-]?{int}
real	(({int}\.?)|({digit}*\.{int})){pow}?
%%

{ws}	{ ; }	// skip blanks and tabs
{int}	{ sscanf(yytext, "%ld", &yylval.inum); return INUM; }
{real}	{ sscanf(yytext, "%lf", &yylval.rnum); return RNUM; }
{var}	{
	char *s = yytext[0] == '\'' ? yytext + 1 : yytext;
	yylval.var = malloc(strlen(s) + 1);
	strcpy(yylval.var, s);
	return VAR; }
[+*()]	{ return yytext[0]; }
\n	{ ++lineno; return '\n'; }
.	{ printf("unknown token %c\n", yytext[0]); }
