%{
#include <stdio.h>
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
{ws}	{ ; }	/* skip blanks and tabs */
{real}	{ return NUM; }
{var}	{ return VAR; }
"+"	{ return PLUS; }
"*"	{ return TIMES; }
"("	{ return LPAR; }
")"	{ return RPAR; }
\n	{ ++lineno; return EOL; }
.	{ printf("unknown token %c\n", yytext[0]); }
