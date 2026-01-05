%{
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"

extern int yylex(void);
extern int yylineno;
void yyerror(const char *s);

ASTNode *ast_root = NULL;

static ASTNode *wrap_optional(ASTNode *node);
static ASTNode *make_binary(const char *op, ASTNode *lhs, ASTNode *rhs);
static ASTNode *make_unary(const char *op, ASTNode *operand);
static ASTNode *create_typed_node_with_line(ASTNodeType type, const char *text, const char *dtype);

/* Helper to create node with current line number */
#define CREATE_NODE(type, text) ast_create_node_with_line(type, text, yylineno)
%}

%union {
    char *str;
    ASTNode *node;
}

%token EGER
%token DEGILSE
%token ICIN
%token IKEN
%token DONDUR
%token INT
%token VOID
%token <str> IDENTIFIER
%token <str> NUMBER
%token <str> STRING_LITERAL
%token EQEQ
%token NEQ
%token LEQ
%token GEQ

%type <node> program external_declaration_list external_declaration function_definition parameter_list parameter_list_opt parameter_declaration compound_statement statement statement_list statement_list_opt expression expression_opt selection_statement iteration_statement jump_statement declaration init_declarator_list init_declarator for_init_statement for_condition_statement for_post_expression expression_statement assignment_expression equality_expression relational_expression additive_expression multiplicative_expression unary_expression primary_expression postfix_expression argument_expression_list argument_expression_list_opt
%type <str> type_specifier

%start program

%right '='
%left EQEQ NEQ
%left '<' '>' LEQ GEQ
%left '+' '-'
%left '*' '/' '%'
%precedence UMINUS

%%

program
    : external_declaration_list
      {
          ASTNode *root = ast_create_node(AST_PROGRAM, NULL);
          if ($1) {
              ast_append_child(root, $1);
          }
          ast_root = root;
          $$ = root;
      }
    ;

external_declaration_list
    : external_declaration
      { $$ = $1; }
    | external_declaration_list external_declaration
      { $$ = ast_append_sibling($1, $2); }
    ;

external_declaration
    : function_definition
      { $$ = $1; }
    | declaration
      { $$ = $1; }
    ;

function_definition
    : type_specifier IDENTIFIER '(' parameter_list_opt ')' compound_statement
      {
          ASTNode *func = create_typed_node_with_line(AST_FUNCTION, $2, $1);
          free($1);
          free($2);
          ASTNode *params = CREATE_NODE(AST_PARAM_LIST, NULL);
          if ($4) {
              ast_append_child(params, $4);
          }
          ast_append_child(func, params);
          ast_append_child(func, $6);
          $$ = func;
      }
    ;

parameter_list_opt
    : parameter_list
      { $$ = $1; }
    | /* empty */
      { $$ = NULL; }
    ;

parameter_list
    : parameter_declaration
      { $$ = $1; }
    | parameter_list ',' parameter_declaration
      { $$ = ast_append_sibling($1, $3); }
    ;

parameter_declaration
    : type_specifier IDENTIFIER
      {
          ASTNode *param = ast_create_typed_node(AST_PARAM, $2, $1);
          free($1);
          free($2);
          $$ = param;
      }
    ;

type_specifier
    : INT
      { $$ = ast_strdup("int"); }
    | VOID
      { $$ = ast_strdup("void"); }
    ;

compound_statement
    : '{' statement_list_opt '}'
      {
          ASTNode *block = ast_create_node(AST_BLOCK, NULL);
          if ($2) {
              ast_append_child(block, $2);
          }
          $$ = block;
      }
    ;

statement_list_opt
    : statement_list
      { $$ = $1; }
    | /* empty */
      { $$ = NULL; }
    ;

statement_list
    : statement
      { $$ = $1; }
    | statement_list statement
      { $$ = ast_append_sibling($1, $2); }
    ;

statement
    : compound_statement
      { $$ = $1; }
    | expression_statement
      { $$ = $1; }
    | selection_statement
      { $$ = $1; }
    | iteration_statement
      { $$ = $1; }
    | jump_statement
      { $$ = $1; }
    | declaration
      { $$ = $1; }
    ;

expression_statement
    : expression_opt ';'
      {
          ASTNode *stmt = ast_create_node(AST_EXPR_STATEMENT, NULL);
          if ($1) {
              ast_append_child(stmt, $1);
          }
          $$ = stmt;
      }
    ;

expression_opt
    : expression
      { $$ = $1; }
    | /* empty */
      { $$ = NULL; }
    ;

selection_statement
    : EGER '(' expression ')' statement
      {
          ASTNode *node = ast_create_node(AST_IF, NULL);
          ast_append_child(node, $3);
          ast_append_child(node, $5);
          $$ = node;
      }
    | EGER '(' expression ')' statement DEGILSE statement
      {
          ASTNode *node = ast_create_node(AST_IF_ELSE, NULL);
          ast_append_child(node, $3);
          ast_append_child(node, $5);
          ast_append_child(node, $7);
          $$ = node;
      }
    ;

iteration_statement
    : IKEN '(' expression ')' statement
      {
          ASTNode *node = ast_create_node(AST_WHILE, NULL);
          ast_append_child(node, $3);
          ast_append_child(node, $5);
          $$ = node;
      }
    | ICIN '(' for_init_statement for_condition_statement for_post_expression ')' statement
      {
          ASTNode *node = ast_create_node(AST_FOR, NULL);
          ast_append_child(node, wrap_optional($3));
          ast_append_child(node, wrap_optional($4));
          ast_append_child(node, wrap_optional($5));
          ast_append_child(node, $7);
          $$ = node;
      }
    ;

for_init_statement
    : expression_opt ';'
      {
          if ($1) {
              ASTNode *stmt = ast_create_node(AST_EXPR_STATEMENT, NULL);
              ast_append_child(stmt, $1);
              $$ = stmt;
          } else {
              $$ = NULL;
          }
      }
    | declaration
      { $$ = $1; }
    ;

for_condition_statement
    : expression_opt ';'
      { $$ = $1; }
    ;

for_post_expression
    : expression_opt
      { $$ = $1; }
    ;

jump_statement
    : DONDUR expression_opt ';'
      {
          ASTNode *node = ast_create_node(AST_RETURN, NULL);
          if ($2) {
              ast_append_child(node, $2);
          }
          $$ = node;
      }
    ;

declaration
    : type_specifier init_declarator_list ';'
      {
          ASTNode *list = $2;
          for (ASTNode *iter = list; iter; iter = iter->next_sibling) {
              ast_set_type(iter, $1);
          }
          free($1);
          $$ = list;
      }
    ;

init_declarator_list
    : init_declarator
      { $$ = $1; }
    | init_declarator_list ',' init_declarator
      { $$ = ast_append_sibling($1, $3); }
    ;

init_declarator
    : IDENTIFIER
      {
          ASTNode *decl = ast_create_node(AST_VAR_DECL, $1);
          free($1);
          $$ = decl;
      }
    | IDENTIFIER '=' assignment_expression
      {
          ASTNode *decl = ast_create_node(AST_VAR_DECL, $1);
          free($1);
          ast_append_child(decl, $3);
          $$ = decl;
      }
    ;

expression
    : assignment_expression
      { $$ = $1; }
    ;

assignment_expression
    : unary_expression '=' assignment_expression
      {
          ASTNode *node = ast_create_node(AST_ASSIGNMENT, "=");
          ast_append_child(node, $1);
          ast_append_child(node, $3);
          $$ = node;
      }
    | equality_expression
      { $$ = $1; }
    ;

equality_expression
    : equality_expression EQEQ relational_expression
      { $$ = make_binary("==", $1, $3); }
    | equality_expression NEQ relational_expression
      { $$ = make_binary("!=", $1, $3); }
    | relational_expression
      { $$ = $1; }
    ;

relational_expression
    : relational_expression '<' additive_expression
      { $$ = make_binary("<", $1, $3); }
    | relational_expression '>' additive_expression
      { $$ = make_binary(">", $1, $3); }
    | relational_expression LEQ additive_expression
      { $$ = make_binary("<=", $1, $3); }
    | relational_expression GEQ additive_expression
      { $$ = make_binary(">=", $1, $3); }
    | additive_expression
      { $$ = $1; }
    ;

additive_expression
    : additive_expression '+' multiplicative_expression
      { $$ = make_binary("+", $1, $3); }
    | additive_expression '-' multiplicative_expression
      { $$ = make_binary("-", $1, $3); }
    | multiplicative_expression
      { $$ = $1; }
    ;

multiplicative_expression
    : multiplicative_expression '*' unary_expression
      { $$ = make_binary("*", $1, $3); }
    | multiplicative_expression '/' unary_expression
      { $$ = make_binary("/", $1, $3); }
    | multiplicative_expression '%' unary_expression
      { $$ = make_binary("%", $1, $3); }
    | unary_expression
      { $$ = $1; }
    ;

unary_expression
    : '-' unary_expression %prec UMINUS
      {
          $$ = make_unary("-", $2);
      }
    | postfix_expression
      { $$ = $1; }
    ;

postfix_expression
    : primary_expression
      { $$ = $1; }
    | IDENTIFIER '(' argument_expression_list_opt ')'
      {
          ASTNode *call = ast_create_node(AST_FUNCTION_CALL, $1);
          free($1);
          ASTNode *args = ast_create_node(AST_ARGUMENT_LIST, NULL);
          if ($3) {
              ast_append_child(args, $3);
          }
          ast_append_child(call, args);
          $$ = call;
      }
    ;

argument_expression_list_opt
    : argument_expression_list
      { $$ = $1; }
    | /* empty */
      { $$ = NULL; }
    ;

argument_expression_list
    : assignment_expression
      { $$ = $1; }
    | argument_expression_list ',' assignment_expression
      { $$ = ast_append_sibling($1, $3); }
    ;

primary_expression
    : IDENTIFIER
      {
          ASTNode *id = ast_create_node(AST_IDENTIFIER, $1);
          free($1);
          $$ = id;
      }
    | NUMBER
      {
          ASTNode *num = ast_create_node(AST_NUMBER_LITERAL, $1);
          free($1);
          $$ = num;
      }
    | STRING_LITERAL
      {
          ASTNode *str = ast_create_node(AST_STRING_LITERAL, $1);
          free($1);
          $$ = str;
      }
    | '(' expression ')'
      { $$ = $2; }
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Sozdizimi hatasi: %s\n", s);
}

static ASTNode *wrap_optional(ASTNode *node) {
    return node ? node : ast_create_node(AST_EMPTY, NULL);
}

static ASTNode *make_binary(const char *op, ASTNode *lhs, ASTNode *rhs) {
    ASTNode *node = CREATE_NODE(AST_BINARY_EXPR, op);
    ast_append_child(node, lhs);
    ast_append_child(node, rhs);
    return node;
}

static ASTNode *make_unary(const char *op, ASTNode *operand) {
    ASTNode *node = CREATE_NODE(AST_UNARY_EXPR, op);
    ast_append_child(node, operand);
    return node;
}

static ASTNode *create_typed_node_with_line(ASTNodeType type, const char *text, const char *dtype) {
    ASTNode *node = ast_create_typed_node(type, text, dtype);
    ast_set_line(node, yylineno);
    return node;
}
