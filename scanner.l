%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.tab.h"

static char *copy_text(const char *text, size_t length);
static char *strip_quotes(const char *text);
int yywrap(void);
%}

%option noinput
%option nounput
%option yylineno

%x COMMENT

%%

"eger"               { return EGER; }
"degilse"            { return DEGILSE; }
"icin"               { return ICIN; }
"iken"               { return IKEN; }
"dondur"             { return DONDUR; }
"int"                { return INT; }
"void"               { return VOID; }

[ \t\r\n]+           { /* Bosluklari atla */ }

"//".*               { /* Tek satirlik yorum */ }
"/*"                 { BEGIN(COMMENT); }
<COMMENT>"*/"        { BEGIN(INITIAL); }
<COMMENT>\n          { /* Satir sayisi otomatik artar */ }
<COMMENT>.           { /* Cok satirli yorum icerigi */ }
<COMMENT><<EOF>>     {
    fprintf(stderr, "Kapanmayan yorum blogu (satir %d)\n", yylineno);
    return 0;
}

"=="                 { return EQEQ; }
"!="                 { return NEQ; }
"<="                 { return LEQ; }
">="                 { return GEQ; }

"="                  { return '='; }
","                  { return ','; }
";"                  { return ';'; }
"("                  { return '('; }
")"                  { return ')'; }
"{"                  { return '{'; }
"}"                  { return '}'; }
"+"                  { return '+'; }
"-"                  { return '-'; }
"*"                  { return '*'; }
"/"                  { return '/'; }
"%"                  { return '%'; }
"<"                  { return '<'; }
">"                  { return '>'; }

\"([^\\\"\n]|\\.)*\" {
    yylval.str = strip_quotes(yytext);
    return STRING_LITERAL;
}

[0-9]+ {
    yylval.str = copy_text(yytext, yyleng);
    return NUMBER;
}

[a-zA-Z_][a-zA-Z0-9_]* {
    yylval.str = copy_text(yytext, yyleng);
    return IDENTIFIER;
}

. {
    fprintf(stderr, "Bilinmeyen sembol: %s (satir %d)\n", yytext, yylineno);
    return yytext[0];
}

%%

int yywrap(void) {
    return 1;
}

static char *copy_text(const char *text, size_t length) {
    char *copy = (char *)malloc(length + 1);
    if (!copy) {
        fprintf(stderr, "Bellek tahsisi basarisiz oldu\n");
        exit(EXIT_FAILURE);
    }
    memcpy(copy, text, length);
    copy[length] = '\0';
    return copy;
}

static char *strip_quotes(const char *text) {
    size_t len = strlen(text);
    if (len < 2) {
        return copy_text(text, len);
    }
    char *value = (char *)malloc(len - 1);
    if (!value) {
        fprintf(stderr, "Bellek tahsisi basarisiz oldu\n");
        exit(EXIT_FAILURE);
    }
    memcpy(value, text + 1, len - 2);
    value[len - 2] = '\0';
    return value;
}