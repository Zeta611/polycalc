%{
#include <stdio.h>
#include <string.h>
#include "poly.h"
#include "y.tab.h"
%}

ws	[ \t]+

ulph	[_a-zA-Z]
ulnum	[_a-zA-Z0-9]
var	{ulph}{ulnum}*

digit	[0-9]
int	{digit}+
pow	[eE][+-]?{int}
real	(({int}\.?)|({digit}*\.{int})){pow}?
%%

{ws}	{ ; }	// skip blanks and tabs
{real}	{ sscanf(yytext, "%lf", &yylval.num); return NUM; }
{var}	{
	yylval.var = malloc(strlen(yytext) + 1);
	strcpy(yylval.var, yytext);
	return VAR; }
[+*()]	{ return yytext[0]; }
\n	{ ++lineno; return '\n'; }
.	{ printf("unknown token %c\n", yytext[0]); }
