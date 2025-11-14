%{
#include <stdio.h>
int yywrap() { return 1; }  // Bu satırı ekle
%}

%%

"eger"          { printf("KEYWORD: eger\n"); }
"degilse"       { printf("KEYWORD: degilse\n"); }
"icin"          { printf("KEYWORD: icin\n"); }
"iken"          { printf("KEYWORD: iken\n"); }
"dondur"        { printf("KEYWORD: dondur\n"); }
"int"           { printf("KEYWORD: int\n"); }
"void"          { printf("KEYWORD: void\n"); }

[a-zA-Z_][a-zA-Z0-9_]*  { printf("IDENTIFIER: %s\n", yytext); }

[0-9]+          { printf("NUMBER: %s\n", yytext); }

\"[^\"]*\"      { printf("STRING: %s\n", yytext); }

"//".*          { /* Tek satırlık yorum, yok say */ }
"/*"([^*]|\*+[^*/])*\*+"/"  { /* Çok satırlık yorum, yok say */ }

"+"             { printf("OPERATOR: +\n"); }
"-"             { printf("OPERATOR: -\n"); }
"*"             { printf("OPERATOR: *\n"); }
"/"             { printf("OPERATOR: /\n"); }
"%"             { printf("OPERATOR: %%\n"); }
"=="            { printf("OPERATOR: ==\n"); }
"!="            { printf("OPERATOR: !=\n"); }
"<"             { printf("OPERATOR: <\n"); }
">"             { printf("OPERATOR: >\n"); }
"<="            { printf("OPERATOR: <=\n"); }
">="            { printf("OPERATOR: >=\n"); }
"="             { printf("OPERATOR: =\n"); }

"("             { printf("PUNCTUATION: (\n"); }
")"             { printf("PUNCTUATION: )\n"); }
"{"             { printf("PUNCTUATION: {\n"); }
"}"             { printf("PUNCTUATION: }\n"); }
";"             { printf("PUNCTUATION: ;\n"); }
","             { printf("PUNCTUATION: ,\n"); }

[ \t\n]+        { /* Whitespace, yok say */ }

.               { printf("UNKNOWN: %s\n", yytext); }

%%

int main() {
    yylex();
    return 0;
}