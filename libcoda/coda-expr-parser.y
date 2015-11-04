/*
 * Copyright (C) 2007-2009 S&T, The Netherlands.
 *
 * This file is part of CODA.
 *
 * CODA is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * CODA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CODA; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* *INDENT-OFF* */

%{

/* *INDENT-ON* */

/* Make parser independent from other parsers */
#define yymaxdepth coda_expr_maxdepth
#define yyparse coda_expr_parse
#define yylex   coda_expr_lex
#define yyerror coda_expr_error
#define yylval  coda_expr_lval
#define yychar  coda_expr_char
#define yydebug coda_expr_debug
#define yypact  coda_expr_pact
#define yyr1    coda_expr_r1
#define yyr2    coda_expr_r2
#define yydef   coda_expr_def
#define yychk   coda_expr_chk
#define yypgo   coda_expr_pgo
#define yyact   coda_expr_act
#define yyexca  coda_expr_exca
#define yyerrflag coda_expr_errflag
#define yynerrs coda_expr_nerrs
#define yyps    coda_expr_ps
#define yypv    coda_expr_pv
#define yys     coda_expr_s
#define yy_yys  coda_expr_yys
#define yystate coda_expr_state
#define yytmp   coda_expr_tmp
#define yyv     coda_expr_v
#define yy_yyv  coda_expr_yyv
#define yyval   coda_expr_val
#define yylloc  coda_expr_lloc
#define yyreds  coda_expr_reds
#define yytoks  coda_expr_toks
#define yylhs   coda_expr_yylhs
#define yylen   coda_expr_yylen
#define yydefred coda_expr_yydefred
#define yydgoto coda_expr_yydgoto
#define yysindex coda_expr_yysindex
#define yyrindex coda_expr_yyrindex
#define yygindex coda_expr_yygindex
#define yytable  coda_expr_yytable
#define yycheck  coda_expr_yycheck
#define yyname   coda_expr_yyname
#define yyrule   coda_expr_yyrule

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "coda-expr-internal.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static coda_Expr *parsed_expression;

/* tokenizer declarations */
int coda_expr_lex(void);
void *coda_expr__scan_string(const char *yy_str);
void coda_expr__delete_buffer(void *);

static void coda_expr_error(const char *error)
{
    coda_set_error(CODA_ERROR_EXPRESSION, error);
}

/* *INDENT-OFF* */

%}

%union{
    char *stringval;
    coda_Expr *expr;
}

%token    <stringval> INT_VALUE
%token    <stringval> FLOAT_VALUE
%token    <stringval> STRING_VALUE
%token    <stringval> NAME
%left                 LOGICAL_AND LOGICAL_OR
%nonassoc             NOT
%left                 GREATER_EQUAL LESS_EQUAL EQUAL NOT_EQUAL '>' '<'
%left                 AND OR
%left                 '+' '-'
%left                 '*' '/' '%'
%nonassoc             '^'
%nonassoc             UNARY
%token                GOTO_PARENT

%token                ASCIILINE
%token                DO
%token                FOR
%token                FORINDEX
%token                STEP
%token                TO
%token                NAN_VALUE
%token                INF_VALUE
%token                TRUE_VALUE
%token                FALSE_VALUE
%token                FUNC_ABS
%token                FUNC_ADD
%token                FUNC_ALL
%token                FUNC_BITOFFSET
%token                FUNC_BITSIZE
%token                FUNC_BOOL
%token                FUNC_BYTES
%token                FUNC_BYTEOFFSET
%token                FUNC_BYTESIZE
%token                FUNC_CEIL
%token                FUNC_COUNT
%token                FUNC_EXISTS
%token                FUNC_FILENAME
%token                FUNC_FILESIZE
%token                FUNC_FLOAT
%token                FUNC_FLOOR
%token                FUNC_GOTO
%token                FUNC_IF
%token                FUNC_INDEX
%token                FUNC_INT
%token                FUNC_ISNAN
%token                FUNC_ISINF
%token                FUNC_ISPLUSINF
%token                FUNC_ISMININF
%token                FUNC_LENGTH
%token                FUNC_LTRIM
%token                FUNC_NUMELEMENTS
%token                FUNC_MAX
%token                FUNC_MIN
%token                FUNC_PRODUCTCLASS
%token                FUNC_PRODUCTTYPE
%token                FUNC_PRODUCTVERSION
%token                FUNC_ROUND
%token                FUNC_RTRIM
%token                FUNC_STR
%token                FUNC_SUBSTR
%token                FUNC_TRIM
%token                FUNC_UNBOUNDINDEX

%type     <expr>      node
%type     <expr>      rootnode
%type     <expr>      nonrootnode
%type     <expr>      voidexpr
%type     <expr>      boolexpr
%type     <expr>      intexpr
%type     <expr>      floatexpr
%type     <expr>      stringexpr
%type     <stringval> identifier
%destructor { coda_expr_delete($$); } node voidexpr boolexpr intexpr floatexpr stringexpr
%destructor { free($$); } INT_VALUE FLOAT_VALUE STRING_VALUE NAME

%error-verbose

/* Expect 3 shift-reduce conflicts */
%expect 3

%%

input:
      voidexpr { parsed_expression = $1; }
    | boolexpr { parsed_expression = $1; }
    | intexpr { parsed_expression = $1; }
    | floatexpr { parsed_expression = $1; }
    | stringexpr { parsed_expression = $1; }

identifier:
      ASCIILINE { $$ = "asciiline"; }
    | DO  { $$ = "do"; }
    | FOR { $$ = "for"; }
    | FORINDEX { $$ = "i"; }
    | STEP { $$ = "step"; }
    | TO { $$ = "to"; }
    | NAN_VALUE { $$ = "nan"; }
    | INF_VALUE { $$ = "inf"; }
    | TRUE_VALUE { $$ = "true"; }
    | FALSE_VALUE { $$ = "false"; }
    | FUNC_ABS { $$ = "abs"; }
    | FUNC_ADD { $$ = "add"; }
    | FUNC_ALL { $$ = "all"; }
    | FUNC_BITOFFSET { $$ = "bitoffset"; }
    | FUNC_BOOL { $$ = "bool"; }
    | FUNC_BYTES { $$ = "bytes"; }
    | FUNC_BYTEOFFSET { $$ = "byteoffset"; }
    | FUNC_BYTESIZE { $$ = "bytesize"; }
    | FUNC_CEIL { $$ = "ceil"; }
    | FUNC_COUNT { $$ = "count"; }
    | FUNC_EXISTS { $$ = "exists"; }
    | FUNC_FILENAME { $$ = "filename"; }
    | FUNC_FILESIZE { $$ = "filesize"; }
    | FUNC_FLOAT { $$ = "float"; }
    | FUNC_FLOOR { $$ = "floor"; }
    | FUNC_GOTO { $$ = "goto"; }
    | FUNC_IF { $$ = "if"; }
    | FUNC_INDEX { $$ = "index"; }
    | FUNC_INT { $$ = "int"; }
    | FUNC_ISNAN { $$ = "isnan"; }
    | FUNC_ISINF { $$ = "isinf"; }
    | FUNC_ISPLUSINF { $$ = "isplusinf"; }
    | FUNC_ISMININF { $$ = "ismininf"; }
    | FUNC_LENGTH { $$ = "length"; }
    | FUNC_LTRIM { $$ = "ltrim"; }
    | FUNC_NUMELEMENTS { $$ = "numelements"; }
    | FUNC_MAX { $$ = "max"; }
    | FUNC_MIN { $$ = "min"; }
    | FUNC_PRODUCTCLASS { $$ = "productclass"; }
    | FUNC_PRODUCTTYPE { $$ = "producttype"; }
    | FUNC_PRODUCTVERSION { $$ = "productversion"; }
    | FUNC_ROUND { $$ = "round"; }
    | FUNC_RTRIM { $$ = "rtrim"; }
    | FUNC_STR { $$ = "str"; }
    | FUNC_SUBSTR { $$ = "substr"; }
    | FUNC_TRIM { $$ = "trim"; }
    | FUNC_UNBOUNDINDEX { $$ = "unboundindex"; }
    ;

voidexpr:
      '$' NAME '=' intexpr {
            $$ = coda_expr_new(expr_variable_set, $2, NULL, $4, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | '$' identifier '=' intexpr {
            $$ = coda_expr_new(expr_variable_set, strdup($2), NULL, $4, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | '$' NAME '[' intexpr ']' '=' intexpr {
            $$ = coda_expr_new(expr_variable_set, $2, $4, $7, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | '$' identifier '[' intexpr ']' '=' intexpr {
            $$ = coda_expr_new(expr_variable_set, strdup($2), $4, $7, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | voidexpr ';' voidexpr {
            $$ = coda_expr_new(expr_sequence, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FOR FORINDEX '=' intexpr TO intexpr DO voidexpr {
            $$ = coda_expr_new(expr_for, NULL, $4, $6, NULL, $8);
            if ($$ == NULL) YYERROR;
        }
    | FOR FORINDEX '=' intexpr TO intexpr STEP intexpr DO voidexpr {
            $$ = coda_expr_new(expr_for, NULL, $4, $6, $8, $10);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_GOTO '(' node ')' {
            $$ = coda_expr_new(expr_goto, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    ;

boolexpr:
     TRUE_VALUE {
            $$ = coda_expr_new(expr_constant_boolean, strdup("true"), NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FALSE_VALUE {
            $$ = coda_expr_new(expr_constant_boolean, strdup("false"), NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | boolexpr LOGICAL_AND boolexpr {
            $$ = coda_expr_new(expr_logical_and, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | boolexpr LOGICAL_OR boolexpr {
            $$ = coda_expr_new(expr_logical_or, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | NOT boolexpr {
            $$ = coda_expr_new(expr_not, NULL, $2, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr GREATER_EQUAL intexpr {
            $$ = coda_expr_new(expr_greater_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr LESS_EQUAL intexpr {
            $$ = coda_expr_new(expr_less_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr EQUAL intexpr {
            $$ = coda_expr_new(expr_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr NOT_EQUAL intexpr {
            $$ = coda_expr_new(expr_not_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr '<' intexpr {
            $$ = coda_expr_new(expr_less, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr '>' intexpr {
            $$ = coda_expr_new(expr_greater, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr GREATER_EQUAL floatexpr {
            $$ = coda_expr_new(expr_greater_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr GREATER_EQUAL intexpr {
            $$ = coda_expr_new(expr_greater_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr GREATER_EQUAL floatexpr {
            $$ = coda_expr_new(expr_greater_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr LESS_EQUAL floatexpr {
            $$ = coda_expr_new(expr_less_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr LESS_EQUAL intexpr {
            $$ = coda_expr_new(expr_less_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr LESS_EQUAL floatexpr {
            $$ = coda_expr_new(expr_less_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr EQUAL floatexpr {
            $$ = coda_expr_new(expr_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr EQUAL intexpr {
            $$ = coda_expr_new(expr_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr EQUAL floatexpr {
            $$ = coda_expr_new(expr_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr NOT_EQUAL floatexpr {
            $$ = coda_expr_new(expr_not_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr NOT_EQUAL intexpr {
            $$ = coda_expr_new(expr_not_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr NOT_EQUAL floatexpr {
            $$ = coda_expr_new(expr_not_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr '<' floatexpr {
            $$ = coda_expr_new(expr_less, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr '<' intexpr {
            $$ = coda_expr_new(expr_less, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr '<' floatexpr {
            $$ = coda_expr_new(expr_less, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr '>' floatexpr {
            $$ = coda_expr_new(expr_greater, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr '>' intexpr {
            $$ = coda_expr_new(expr_greater, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr '>' floatexpr {
            $$ = coda_expr_new(expr_greater, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | stringexpr EQUAL stringexpr {
            $$ = coda_expr_new(expr_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | stringexpr NOT_EQUAL stringexpr {
            $$ = coda_expr_new(expr_not_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | stringexpr '<' stringexpr {
            $$ = coda_expr_new(expr_less, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | stringexpr '>' stringexpr {
            $$ = coda_expr_new(expr_greater, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | stringexpr GREATER_EQUAL stringexpr {
            $$ = coda_expr_new(expr_greater_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | stringexpr LESS_EQUAL stringexpr {
            $$ = coda_expr_new(expr_less_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | '(' boolexpr ')' { $$ = $2; }
    | FUNC_ISNAN '(' floatexpr ')' {
            $$ = coda_expr_new(expr_isnan, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_ISINF '(' floatexpr ')' {
            $$ = coda_expr_new(expr_isinf, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_ISPLUSINF '(' floatexpr ')' {
            $$ = coda_expr_new(expr_isplusinf, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_ISMININF '(' floatexpr ')' {
            $$ = coda_expr_new(expr_ismininf, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_EXISTS '(' node ')' {
            $$ = coda_expr_new(expr_exists, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_EXISTS '(' node ',' boolexpr ')' {
            $$ = coda_expr_new(expr_array_exists, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_EXISTS '(' '$' NAME ',' boolexpr ')' {
            $$ = coda_expr_new(expr_variable_exists, $4, $6, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_EXISTS '(' '$' identifier ',' boolexpr ')' {
            $$ = coda_expr_new(expr_variable_exists, strdup($4), $6, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_ALL '(' node ',' boolexpr ')' {
            $$ = coda_expr_new(expr_array_all, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    ;

intexpr:
      INT_VALUE {
            $$ = coda_expr_new(expr_constant_integer, $1, NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_INT '(' node ')' {
            $$ = coda_expr_new(expr_integer, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_INT '(' stringexpr ')' {
            $$ = coda_expr_new(expr_integer, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | '$' NAME {
            $$ = coda_expr_new(expr_variable_value, $2, NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | '$' identifier {
            $$ = coda_expr_new(expr_variable_value, strdup($2), NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | '$' NAME '[' intexpr ']' {
            $$ = coda_expr_new(expr_variable_value, $2, $4, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | '$' identifier '[' intexpr ']' {
            $$ = coda_expr_new(expr_variable_value, strdup($2), $4, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FORINDEX {
            $$ = coda_expr_new(expr_for_index, NULL, NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | '-' intexpr %prec UNARY {
            $$ = coda_expr_new(expr_neg, NULL, $2, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | '+' intexpr %prec UNARY { $$ = $2; }
    | intexpr '+' intexpr {
            $$ = coda_expr_new(expr_add, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr '-' intexpr {
            $$ = coda_expr_new(expr_subtract, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr '*' intexpr {
            $$ = coda_expr_new(expr_multiply, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr '/' intexpr {
            $$ = coda_expr_new(expr_divide, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr '%' intexpr {
            $$ = coda_expr_new(expr_modulo, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr '^' intexpr {
            $$ = coda_expr_new(expr_power, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr LOGICAL_AND intexpr {
            $$ = coda_expr_new(expr_and, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr LOGICAL_OR intexpr {
            $$ = coda_expr_new(expr_or, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | '(' intexpr ')' { $$ = $2; }
    | FUNC_ABS '(' intexpr ')' {
            $$ = coda_expr_new(expr_abs, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_MAX '(' intexpr ',' intexpr ')' {
            $$ = coda_expr_new(expr_max, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_MIN '(' intexpr ',' intexpr ')' {
            $$ = coda_expr_new(expr_min, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_NUMELEMENTS '(' node ')' {
            $$ = coda_expr_new(expr_num_elements, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_COUNT '(' node ',' boolexpr ')' {
            $$ = coda_expr_new(expr_array_count, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_ADD '(' node ',' intexpr ')' {
            $$ = coda_expr_new(expr_array_add, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_LENGTH '(' stringexpr ')' {
            $$ = coda_expr_new(expr_length, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_BITSIZE '(' node ')' {
            $$ = coda_expr_new(expr_bit_size, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_BYTESIZE '(' node ')' {
            $$ = coda_expr_new(expr_byte_size, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_PRODUCTVERSION '(' ')' {
            $$ = coda_expr_new(expr_product_version, NULL, NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_FILESIZE '(' ')' {
            $$ = coda_expr_new(expr_file_size, NULL, NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_BITOFFSET '(' node ')' {
            $$ = coda_expr_new(expr_bit_offset, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_BYTEOFFSET '(' node ')' {
            $$ = coda_expr_new(expr_byte_offset, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_INDEX '(' node ')' {
            $$ = coda_expr_new(expr_index, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_INDEX '(' node ',' boolexpr ')' {
            $$ = coda_expr_new(expr_array_index, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_INDEX '(' '$' NAME ',' boolexpr ')' {
            $$ = coda_expr_new(expr_variable_index, $4, $6, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_INDEX '(' '$' identifier ',' boolexpr ')' {
            $$ = coda_expr_new(expr_variable_index, strdup($4), $6, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_IF '(' boolexpr ',' intexpr ',' intexpr ')' {
            $$ = coda_expr_new(expr_if, NULL, $3, $5, $7, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_UNBOUNDINDEX '(' node ',' boolexpr ')' {
            $$ = coda_expr_new(expr_unbound_array_index, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    ;

floatexpr:
      FLOAT_VALUE {
            $$ = coda_expr_new(expr_constant_double, $1, NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | NAN_VALUE {
            $$ = coda_expr_new(expr_constant_double, strdup("nan"), NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | INF_VALUE {
            $$ = coda_expr_new(expr_constant_double, strdup("inf"), NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_FLOAT '(' node ')' {
            $$ = coda_expr_new(expr_float, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_FLOAT '(' intexpr ')' {
            $$ = coda_expr_new(expr_float, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_FLOAT '(' stringexpr ')' {
            $$ = coda_expr_new(expr_float, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | '-' floatexpr %prec UNARY {
            $$ = coda_expr_new(expr_neg, NULL, $2, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | '+' floatexpr %prec UNARY { $$ = $2; }
    | floatexpr '+' floatexpr {
            $$ = coda_expr_new(expr_add, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr '+' intexpr {
            $$ = coda_expr_new(expr_add, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr '+' floatexpr {
            $$ = coda_expr_new(expr_add, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr '-' floatexpr {
            $$ = coda_expr_new(expr_subtract, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr '-' intexpr {
            $$ = coda_expr_new(expr_subtract, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr '-' floatexpr {
            $$ = coda_expr_new(expr_subtract, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr '*' floatexpr {
            $$ = coda_expr_new(expr_multiply, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr '*' intexpr {
            $$ = coda_expr_new(expr_multiply, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr '*' floatexpr {
            $$ = coda_expr_new(expr_multiply, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr '/' floatexpr {
            $$ = coda_expr_new(expr_divide, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr '/' intexpr {
            $$ = coda_expr_new(expr_divide, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr '/' floatexpr {
            $$ = coda_expr_new(expr_divide, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr '%' floatexpr {
            $$ = coda_expr_new(expr_modulo, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr '%' intexpr {
            $$ = coda_expr_new(expr_modulo, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr '%' floatexpr {
            $$ = coda_expr_new(expr_modulo, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr '^' floatexpr {
            $$ = coda_expr_new(expr_power, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr '^' intexpr {
            $$ = coda_expr_new(expr_power, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr '^' floatexpr {
            $$ = coda_expr_new(expr_power, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | '(' floatexpr ')' { $$ = $2; }
    | FUNC_ABS '(' floatexpr ')' {
            $$ = coda_expr_new(expr_abs, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_CEIL '(' floatexpr ')' {
            $$ = coda_expr_new(expr_ceil, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_FLOOR '(' floatexpr ')' {
            $$ = coda_expr_new(expr_floor, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_ROUND '(' floatexpr ')' {
            $$ = coda_expr_new(expr_round, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_MAX '(' floatexpr ',' floatexpr ')' {
            $$ = coda_expr_new(expr_max, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_MAX '(' floatexpr ',' intexpr ')' {
            $$ = coda_expr_new(expr_max, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_MAX '(' intexpr ',' floatexpr ')' {
            $$ = coda_expr_new(expr_max, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_MIN '(' floatexpr ',' floatexpr ')' {
            $$ = coda_expr_new(expr_min, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_MIN '(' floatexpr ',' intexpr ')' {
            $$ = coda_expr_new(expr_min, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_MIN '(' intexpr ',' floatexpr ')' {
            $$ = coda_expr_new(expr_min, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_ADD '(' node ',' floatexpr ')' {
            $$ = coda_expr_new(expr_array_add, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_IF '(' boolexpr ',' floatexpr ',' floatexpr ')' {
            $$ = coda_expr_new(expr_if, NULL, $3, $5, $7, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_IF '(' boolexpr ',' floatexpr ',' intexpr ')' {
            $$ = coda_expr_new(expr_if, NULL, $3, $5, $7, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_IF '(' boolexpr ',' intexpr ',' floatexpr ')' {
            $$ = coda_expr_new(expr_if, NULL, $3, $5, $7, NULL);
            if ($$ == NULL) YYERROR;
        }
    ;

stringexpr:
      STRING_VALUE {
            $$ = coda_expr_new(expr_constant_string, $1, NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_STR '(' node ')' {
            $$ = coda_expr_new(expr_string, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_STR '(' node ',' intexpr ')' {
            $$ = coda_expr_new(expr_string, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_BYTES '(' node ')' {
            $$ = coda_expr_new(expr_bytes, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_BYTES '(' node ',' intexpr ')' {
            $$ = coda_expr_new(expr_bytes, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | stringexpr '+' stringexpr {
            $$ = coda_expr_new(expr_add, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_SUBSTR '(' intexpr ',' intexpr ',' stringexpr ')' {
            $$ = coda_expr_new(expr_substr, NULL, $3, $5, $7, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_LTRIM '(' stringexpr ')' {
            $$ = coda_expr_new(expr_ltrim, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_RTRIM '(' stringexpr ')' {
            $$ = coda_expr_new(expr_rtrim, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_TRIM '(' stringexpr ')' {
            $$ = coda_expr_new(expr_trim, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_ADD '(' node ',' stringexpr ')' {
            $$ = coda_expr_new(expr_array_add, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_PRODUCTCLASS '(' ')' {
            $$ = coda_expr_new(expr_product_class, NULL, NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_PRODUCTTYPE '(' ')' {
            $$ = coda_expr_new(expr_product_type, NULL, NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_FILENAME '(' ')' {
            $$ = coda_expr_new(expr_filename, NULL, NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_IF '(' boolexpr ',' stringexpr ',' stringexpr ')' {
            $$ = coda_expr_new(expr_if, NULL, $3, $5, $7, NULL);
            if ($$ == NULL) YYERROR;
        }
    ;

rootnode:
      '/' {
            $$ = coda_expr_new(expr_goto_root, NULL, NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    ;

nonrootnode:
      '.' {
            $$ = coda_expr_new(expr_goto_here, NULL, NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | ':' {
            $$ = coda_expr_new(expr_goto_begin, NULL, NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | GOTO_PARENT {
            $$ = coda_expr_new(expr_goto_parent, NULL, NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | ASCIILINE {
            $$ = coda_expr_new(expr_asciiline, NULL, NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | rootnode NAME {
            $$ = coda_expr_new(expr_goto_field, $2, $1, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | rootnode identifier {
            $$ = coda_expr_new(expr_goto_field, strdup($2), $1, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | nonrootnode '/' GOTO_PARENT {
            $$ = coda_expr_new(expr_goto_parent, NULL, $1, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | nonrootnode '/' NAME {
            $$ = coda_expr_new(expr_goto_field, $3, $1, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | nonrootnode '/' identifier {
            $$ = coda_expr_new(expr_goto_field, strdup($3), $1, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | node '[' intexpr ']' {
            $$ = coda_expr_new(expr_goto_array_element, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | node '@' NAME {
            $$ = coda_expr_new(expr_goto_attribute, $3, $1, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | node '@' identifier {
            $$ = coda_expr_new(expr_goto_attribute, strdup($3), $1, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    ;

node:
      rootnode { $$ = $1; }
    | nonrootnode { $$ = $1; }
    ;

%%

/* *INDENT-ON* */

int coda_expr_from_string(const char *exprstring, coda_Expr **expr)
{
    void *bufstate;

    parsed_expression = NULL;
    bufstate = (void *)coda_expr__scan_string(exprstring);
    if (coda_expr_parse() != 0)
    {
        if (coda_errno == 0)
        {
            coda_set_error(CODA_ERROR_EXPRESSION, NULL);
        }
        coda_expr__delete_buffer(bufstate);
        return -1;
    }
    coda_expr__delete_buffer(bufstate);
    *expr = parsed_expression;

    return 0;
}
