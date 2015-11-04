/*
 * Copyright (C) 2007-2013 S[&]T, The Netherlands.
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
#define yymaxdepth coda_expression_maxdepth
#define yyparse coda_expression_parse
#define yylex   coda_expression_lex
#define yyerror coda_expression_error
#define yylval  coda_expression_lval
#define yychar  coda_expression_char
#define yydebug coda_expression_debug
#define yypact  coda_expression_pact
#define yyr1    coda_expression_r1
#define yyr2    coda_expression_r2
#define yydef   coda_expression_def
#define yychk   coda_expression_chk
#define yypgo   coda_expression_pgo
#define yyact   coda_expression_act
#define yyexca  coda_expression_exca
#define yyerrflag coda_expression_errflag
#define yynerrs coda_expression_nerrs
#define yyps    coda_expression_ps
#define yypv    coda_expression_pv
#define yys     coda_expression_s
#define yy_yys  coda_expression_yys
#define yystate coda_expression_state
#define yytmp   coda_expression_tmp
#define yyv     coda_expression_v
#define yy_yyv  coda_expression_yyv
#define yyval   coda_expression_val
#define yylloc  coda_expression_lloc
#define yyreds  coda_expression_reds
#define yytoks  coda_expression_toks
#define yylhs   coda_expression_yylhs
#define yylen   coda_expression_yylen
#define yydefred coda_expression_yydefred
#define yydgoto coda_expression_yydgoto
#define yysindex coda_expression_yysindex
#define yyrindex coda_expression_yyrindex
#define yygindex coda_expression_yygindex
#define yytable  coda_expression_yytable
#define yycheck  coda_expression_yycheck
#define yyname   coda_expression_yyname
#define yyrule   coda_expression_yyrule

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "coda-expr.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static coda_expression *parsed_expression;

/* tokenizer declarations */
int coda_expression_lex(void);
void *coda_expression__scan_string(const char *yy_str);
void coda_expression__delete_buffer(void *);

static void coda_expression_error(const char *error)
{
    coda_set_error(CODA_ERROR_EXPRESSION, "%s", error);
}

/* *INDENT-OFF* */

%}

%union{
    char *stringval;
    struct coda_expression_struct *expr;
}

%token    <stringval> INT_VALUE
%token    <stringval> FLOAT_VALUE
%token    <stringval> STRING_VALUE
%token    <stringval> NAME
%token    <stringval> INDEX_VAR
%left                 LOGICAL_AND LOGICAL_OR
%nonassoc             NOT
%left                 GREATER_EQUAL LESS_EQUAL EQUAL NOT_EQUAL '>' '<'
%left                 AND OR
%left                 '+' '-'
%left                 '*' '/' '%'
%nonassoc             '^'
%nonassoc             UNARY
%token                GOTO_PARENT

%token                RAW_PREFIX
%token                ASCIILINE
%token                DO
%token                FOR
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
%token                FUNC_PRODUCTFORMAT
%token                FUNC_PRODUCTTYPE
%token                FUNC_PRODUCTVERSION
%token                FUNC_REGEX
%token                FUNC_ROUND
%token                FUNC_RTRIM
%token                FUNC_STR
%token                FUNC_STRTIME
%token                FUNC_SUBSTR
%token                FUNC_TIME
%token                FUNC_TRIM
%token                FUNC_UNBOUNDINDEX
%token                FUNC_WITH

%type     <expr>      node
%type     <expr>      rootnode
%type     <expr>      nonrootnode
%type     <expr>      voidexpr
%type     <expr>      boolexpr
%type     <expr>      intexpr
%type     <expr>      floatexpr
%type     <expr>      stringexpr
%type     <stringval> identifier
%type     <stringval> reserved_identifier
%destructor { coda_expression_delete($$); } node voidexpr boolexpr intexpr floatexpr stringexpr
%destructor { free($$); } INT_VALUE FLOAT_VALUE STRING_VALUE NAME identifier

%error-verbose

/* Expect 3 shift-reduce conflicts */
%expect 3

%%

input:
      boolexpr { parsed_expression = $1; }
    | intexpr { parsed_expression = $1; }
    | floatexpr { parsed_expression = $1; }
    | stringexpr { parsed_expression = $1; }
    | voidexpr { parsed_expression = $1; }
    | node { parsed_expression = $1; }

reserved_identifier:
      RAW_PREFIX { $$ = "r"; }
    | ASCIILINE { $$ = "asciiline"; }
    | DO  { $$ = "do"; }
    | FOR { $$ = "for"; }
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
    | FUNC_PRODUCTFORMAT { $$ = "productformat"; }
    | FUNC_PRODUCTTYPE { $$ = "producttype"; }
    | FUNC_PRODUCTVERSION { $$ = "productversion"; }
    | FUNC_REGEX { $$ = "regex"; }
    | FUNC_ROUND { $$ = "round"; }
    | FUNC_RTRIM { $$ = "rtrim"; }
    | FUNC_STR { $$ = "str"; }
    | FUNC_STRTIME { $$ = "strtime"; }
    | FUNC_SUBSTR { $$ = "substr"; }
    | FUNC_TIME { $$ = "time"; }
    | FUNC_TRIM { $$ = "trim"; }
    | FUNC_UNBOUNDINDEX { $$ = "unboundindex"; }
    | FUNC_WITH { $$ = "with"; }
    | INDEX_VAR
    ;

identifier:
      NAME
    | reserved_identifier { $$ = strdup($1); }
    ;

voidexpr:
      '$' identifier '=' intexpr {
            $$ = coda_expression_new(expr_variable_set, $2, NULL, $4, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | '$' identifier '[' intexpr ']' '=' intexpr {
            $$ = coda_expression_new(expr_variable_set, $2, $4, $7, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | voidexpr ';' voidexpr {
            $$ = coda_expression_new(expr_sequence, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FOR INDEX_VAR '=' intexpr TO intexpr DO voidexpr {
            $$ = coda_expression_new(expr_for, $2, $4, $6, NULL, $8);
            if ($$ == NULL) YYERROR;
        }
    | FOR INDEX_VAR '=' intexpr TO intexpr STEP intexpr DO voidexpr {
            $$ = coda_expression_new(expr_for, $2, $4, $6, $8, $10);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_GOTO '(' node ')' {
            $$ = coda_expression_new(expr_goto, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_WITH '(' INDEX_VAR '=' intexpr ',' voidexpr ')' {
            $$ = coda_expression_new(expr_with, $3, $5, $7, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    ;

boolexpr:
     TRUE_VALUE {
            $$ = coda_expression_new(expr_constant_boolean, strdup("true"), NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FALSE_VALUE {
            $$ = coda_expression_new(expr_constant_boolean, strdup("false"), NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | boolexpr LOGICAL_AND boolexpr {
            $$ = coda_expression_new(expr_logical_and, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | boolexpr LOGICAL_OR boolexpr {
            $$ = coda_expression_new(expr_logical_or, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | NOT boolexpr {
            $$ = coda_expression_new(expr_not, NULL, $2, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr GREATER_EQUAL intexpr {
            $$ = coda_expression_new(expr_greater_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr LESS_EQUAL intexpr {
            $$ = coda_expression_new(expr_less_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr EQUAL intexpr {
            $$ = coda_expression_new(expr_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr NOT_EQUAL intexpr {
            $$ = coda_expression_new(expr_not_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr '<' intexpr {
            $$ = coda_expression_new(expr_less, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr '>' intexpr {
            $$ = coda_expression_new(expr_greater, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr GREATER_EQUAL floatexpr {
            $$ = coda_expression_new(expr_greater_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr GREATER_EQUAL intexpr {
            $$ = coda_expression_new(expr_greater_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr GREATER_EQUAL floatexpr {
            $$ = coda_expression_new(expr_greater_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr LESS_EQUAL floatexpr {
            $$ = coda_expression_new(expr_less_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr LESS_EQUAL intexpr {
            $$ = coda_expression_new(expr_less_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr LESS_EQUAL floatexpr {
            $$ = coda_expression_new(expr_less_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr EQUAL floatexpr {
            $$ = coda_expression_new(expr_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr EQUAL intexpr {
            $$ = coda_expression_new(expr_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr EQUAL floatexpr {
            $$ = coda_expression_new(expr_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr NOT_EQUAL floatexpr {
            $$ = coda_expression_new(expr_not_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr NOT_EQUAL intexpr {
            $$ = coda_expression_new(expr_not_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr NOT_EQUAL floatexpr {
            $$ = coda_expression_new(expr_not_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr '<' floatexpr {
            $$ = coda_expression_new(expr_less, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr '<' intexpr {
            $$ = coda_expression_new(expr_less, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr '<' floatexpr {
            $$ = coda_expression_new(expr_less, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr '>' floatexpr {
            $$ = coda_expression_new(expr_greater, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr '>' intexpr {
            $$ = coda_expression_new(expr_greater, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr '>' floatexpr {
            $$ = coda_expression_new(expr_greater, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | stringexpr EQUAL stringexpr {
            $$ = coda_expression_new(expr_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | stringexpr NOT_EQUAL stringexpr {
            $$ = coda_expression_new(expr_not_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | stringexpr '<' stringexpr {
            $$ = coda_expression_new(expr_less, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | stringexpr '>' stringexpr {
            $$ = coda_expression_new(expr_greater, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | stringexpr GREATER_EQUAL stringexpr {
            $$ = coda_expression_new(expr_greater_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | stringexpr LESS_EQUAL stringexpr {
            $$ = coda_expression_new(expr_less_equal, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | '(' boolexpr ')' { $$ = $2; }
    | FUNC_ISNAN '(' floatexpr ')' {
            $$ = coda_expression_new(expr_isnan, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_ISINF '(' floatexpr ')' {
            $$ = coda_expression_new(expr_isinf, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_ISPLUSINF '(' floatexpr ')' {
            $$ = coda_expression_new(expr_isplusinf, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_ISMININF '(' floatexpr ')' {
            $$ = coda_expression_new(expr_ismininf, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_REGEX '(' stringexpr ',' stringexpr ')' {
            $$ = coda_expression_new(expr_regex, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_EXISTS '(' node ')' {
            $$ = coda_expression_new(expr_exists, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_EXISTS '(' node ',' boolexpr ')' {
            $$ = coda_expression_new(expr_array_exists, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_EXISTS '(' '$' identifier ',' boolexpr ')' {
            $$ = coda_expression_new(expr_variable_exists, $4, $6, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_ALL '(' node ',' boolexpr ')' {
            $$ = coda_expression_new(expr_array_all, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_IF '(' boolexpr ',' boolexpr ',' boolexpr ')' {
            $$ = coda_expression_new(expr_if, NULL, $3, $5, $7, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_WITH '(' INDEX_VAR '=' intexpr ',' boolexpr ')' {
            $$ = coda_expression_new(expr_with, $3, $5, $7, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    ;

intexpr:
      INT_VALUE {
            $$ = coda_expression_new(expr_constant_integer, $1, NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_INT '(' intexpr ')' {
            $$ = $3;
        }
    | FUNC_INT '(' node ')' {
            $$ = coda_expression_new(expr_integer, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_INT '(' stringexpr ')' {
            $$ = coda_expression_new(expr_integer, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | '$' identifier {
            $$ = coda_expression_new(expr_variable_value, $2, NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | '$' identifier '[' intexpr ']' {
            $$ = coda_expression_new(expr_variable_value, $2, $4, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | INDEX_VAR {
            $$ = coda_expression_new(expr_index_var, $1, NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | '-' intexpr %prec UNARY {
            $$ = coda_expression_new(expr_neg, NULL, $2, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | '+' intexpr %prec UNARY { $$ = $2; }
    | intexpr '+' intexpr {
            $$ = coda_expression_new(expr_add, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr '-' intexpr {
            $$ = coda_expression_new(expr_subtract, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr '*' intexpr {
            $$ = coda_expression_new(expr_multiply, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr '/' intexpr {
            $$ = coda_expression_new(expr_divide, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr '%' intexpr {
            $$ = coda_expression_new(expr_modulo, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr AND intexpr {
            $$ = coda_expression_new(expr_and, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr OR intexpr {
            $$ = coda_expression_new(expr_or, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | '(' intexpr ')' { $$ = $2; }
    | FUNC_ABS '(' intexpr ')' {
            $$ = coda_expression_new(expr_abs, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_MAX '(' intexpr ',' intexpr ')' {
            $$ = coda_expression_new(expr_max, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_MIN '(' intexpr ',' intexpr ')' {
            $$ = coda_expression_new(expr_min, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_NUMELEMENTS '(' node ')' {
            $$ = coda_expression_new(expr_num_elements, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_COUNT '(' node ',' boolexpr ')' {
            $$ = coda_expression_new(expr_array_count, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_ADD '(' node ',' intexpr ')' {
            $$ = coda_expression_new(expr_array_add, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_LENGTH '(' stringexpr ')' {
            $$ = coda_expression_new(expr_length, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_LENGTH '(' node ')' {
            $$ = coda_expression_new(expr_length, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_BITSIZE '(' node ')' {
            $$ = coda_expression_new(expr_bit_size, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_BYTESIZE '(' node ')' {
            $$ = coda_expression_new(expr_byte_size, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_PRODUCTVERSION '(' ')' {
            $$ = coda_expression_new(expr_product_version, NULL, NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_FILESIZE '(' ')' {
            $$ = coda_expression_new(expr_file_size, NULL, NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_BITOFFSET '(' node ')' {
            $$ = coda_expression_new(expr_bit_offset, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_BYTEOFFSET '(' node ')' {
            $$ = coda_expression_new(expr_byte_offset, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_INDEX '(' node ')' {
            $$ = coda_expression_new(expr_index, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_INDEX '(' node ',' boolexpr ')' {
            $$ = coda_expression_new(expr_array_index, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_INDEX '(' '$' identifier ',' boolexpr ')' {
            $$ = coda_expression_new(expr_variable_index, $4, $6, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_IF '(' boolexpr ',' intexpr ',' intexpr ')' {
            $$ = coda_expression_new(expr_if, NULL, $3, $5, $7, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_UNBOUNDINDEX '(' node ',' boolexpr ')' {
            $$ = coda_expression_new(expr_unbound_array_index, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_UNBOUNDINDEX '(' node ',' boolexpr ',' boolexpr ')' {
            $$ = coda_expression_new(expr_unbound_array_index, NULL, $3, $5, $7, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_WITH '(' INDEX_VAR '=' intexpr ',' intexpr ')' {
            $$ = coda_expression_new(expr_with, $3, $5, $7, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    ;

floatexpr:
      FLOAT_VALUE {
            $$ = coda_expression_new(expr_constant_float, $1, NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | NAN_VALUE {
            $$ = coda_expression_new(expr_constant_float, strdup("nan"), NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | INF_VALUE {
            $$ = coda_expression_new(expr_constant_float, strdup("inf"), NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_FLOAT '(' floatexpr ')' {
            $$ = $3;
        }
    | FUNC_FLOAT '(' node ')' {
            $$ = coda_expression_new(expr_float, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_FLOAT '(' intexpr ')' {
            $$ = coda_expression_new(expr_float, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_FLOAT '(' stringexpr ')' {
            $$ = coda_expression_new(expr_float, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | '-' floatexpr %prec UNARY {
            $$ = coda_expression_new(expr_neg, NULL, $2, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | '+' floatexpr %prec UNARY { $$ = $2; }
    | floatexpr '+' floatexpr {
            $$ = coda_expression_new(expr_add, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr '+' intexpr {
            $$ = coda_expression_new(expr_add, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr '+' floatexpr {
            $$ = coda_expression_new(expr_add, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr '-' floatexpr {
            $$ = coda_expression_new(expr_subtract, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr '-' intexpr {
            $$ = coda_expression_new(expr_subtract, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr '-' floatexpr {
            $$ = coda_expression_new(expr_subtract, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr '*' floatexpr {
            $$ = coda_expression_new(expr_multiply, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr '*' intexpr {
            $$ = coda_expression_new(expr_multiply, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr '*' floatexpr {
            $$ = coda_expression_new(expr_multiply, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr '/' floatexpr {
            $$ = coda_expression_new(expr_divide, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr '/' intexpr {
            $$ = coda_expression_new(expr_divide, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr '/' floatexpr {
            $$ = coda_expression_new(expr_divide, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr '%' floatexpr {
            $$ = coda_expression_new(expr_modulo, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr '%' intexpr {
            $$ = coda_expression_new(expr_modulo, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr '%' floatexpr {
            $$ = coda_expression_new(expr_modulo, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr '^' floatexpr {
            $$ = coda_expression_new(expr_power, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | floatexpr '^' intexpr {
            $$ = coda_expression_new(expr_power, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr '^' floatexpr {
            $$ = coda_expression_new(expr_power, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | intexpr '^' intexpr {
            $$ = coda_expression_new(expr_power, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | '(' floatexpr ')' { $$ = $2; }
    | FUNC_ABS '(' floatexpr ')' {
            $$ = coda_expression_new(expr_abs, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_CEIL '(' floatexpr ')' {
            $$ = coda_expression_new(expr_ceil, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_FLOOR '(' floatexpr ')' {
            $$ = coda_expression_new(expr_floor, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_ROUND '(' floatexpr ')' {
            $$ = coda_expression_new(expr_round, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_MAX '(' floatexpr ',' floatexpr ')' {
            $$ = coda_expression_new(expr_max, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_MAX '(' floatexpr ',' intexpr ')' {
            $$ = coda_expression_new(expr_max, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_MAX '(' intexpr ',' floatexpr ')' {
            $$ = coda_expression_new(expr_max, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_MIN '(' floatexpr ',' floatexpr ')' {
            $$ = coda_expression_new(expr_min, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_MIN '(' floatexpr ',' intexpr ')' {
            $$ = coda_expression_new(expr_min, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_MIN '(' intexpr ',' floatexpr ')' {
            $$ = coda_expression_new(expr_min, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_TIME '(' stringexpr ',' stringexpr ')' {
            $$ = coda_expression_new(expr_time, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_ADD '(' node ',' floatexpr ')' {
            $$ = coda_expression_new(expr_array_add, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_IF '(' boolexpr ',' floatexpr ',' floatexpr ')' {
            $$ = coda_expression_new(expr_if, NULL, $3, $5, $7, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_IF '(' boolexpr ',' floatexpr ',' intexpr ')' {
            $$ = coda_expression_new(expr_if, NULL, $3, $5, $7, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_IF '(' boolexpr ',' intexpr ',' floatexpr ')' {
            $$ = coda_expression_new(expr_if, NULL, $3, $5, $7, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_WITH '(' INDEX_VAR '=' intexpr ',' floatexpr ')' {
            $$ = coda_expression_new(expr_with, $3, $5, $7, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    ;

stringexpr:
      STRING_VALUE {
            $$ = coda_expression_new(expr_constant_string, $1, NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | RAW_PREFIX STRING_VALUE {
            $$ = coda_expression_new(expr_constant_rawstring, $2, NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_STR '(' stringexpr ')' {
            $$ = $3;
        }
    | FUNC_STR '(' intexpr ')' {
            $$ = coda_expression_new(expr_string, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_STR '(' node ')' {
            $$ = coda_expression_new(expr_string, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_STR '(' node ',' intexpr ')' {
            $$ = coda_expression_new(expr_string, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_BYTES '(' node ')' {
            $$ = coda_expression_new(expr_bytes, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_BYTES '(' node ',' intexpr ')' {
            $$ = coda_expression_new(expr_bytes, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | stringexpr '+' stringexpr {
            $$ = coda_expression_new(expr_add, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_SUBSTR '(' intexpr ',' intexpr ',' stringexpr ')' {
            $$ = coda_expression_new(expr_substr, NULL, $3, $5, $7, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_LTRIM '(' stringexpr ')' {
            $$ = coda_expression_new(expr_ltrim, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_RTRIM '(' stringexpr ')' {
            $$ = coda_expression_new(expr_rtrim, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_TRIM '(' stringexpr ')' {
            $$ = coda_expression_new(expr_trim, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_ADD '(' node ',' stringexpr ')' {
            $$ = coda_expression_new(expr_array_add, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_REGEX '(' stringexpr ',' stringexpr ',' intexpr ')' {
            $$ = coda_expression_new(expr_regex, NULL, $3, $5, $7, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_REGEX '(' stringexpr ',' stringexpr ',' stringexpr ')' {
            $$ = coda_expression_new(expr_regex, NULL, $3, $5, $7, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_PRODUCTCLASS '(' ')' {
            $$ = coda_expression_new(expr_product_class, NULL, NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_PRODUCTFORMAT '(' ')' {
            $$ = coda_expression_new(expr_product_format, NULL, NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_PRODUCTTYPE '(' ')' {
            $$ = coda_expression_new(expr_product_type, NULL, NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_FILENAME '(' ')' {
            $$ = coda_expression_new(expr_filename, NULL, NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_STRTIME '(' intexpr ')' {
            $$ = coda_expression_new(expr_strtime, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_STRTIME '(' floatexpr ')' {
            $$ = coda_expression_new(expr_strtime, NULL, $3, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_STRTIME '(' intexpr ',' stringexpr ')' {
            $$ = coda_expression_new(expr_strtime, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_STRTIME '(' floatexpr ',' stringexpr ')' {
            $$ = coda_expression_new(expr_strtime, NULL, $3, $5, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_IF '(' boolexpr ',' stringexpr ',' stringexpr ')' {
            $$ = coda_expression_new(expr_if, NULL, $3, $5, $7, NULL);
            if ($$ == NULL) YYERROR;
        }
    | FUNC_WITH '(' INDEX_VAR '=' intexpr ',' stringexpr ')' {
            $$ = coda_expression_new(expr_with, $3, $5, $7, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    ;

rootnode:
      '/' {
            $$ = coda_expression_new(expr_goto_root, NULL, NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    ;

nonrootnode:
      '.' {
            $$ = coda_expression_new(expr_goto_here, NULL, NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | ':' {
            $$ = coda_expression_new(expr_goto_begin, NULL, NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | GOTO_PARENT {
            $$ = coda_expression_new(expr_goto_parent, NULL, NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | ASCIILINE {
            $$ = coda_expression_new(expr_asciiline, NULL, NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | rootnode identifier {
            $$ = coda_expression_new(expr_goto_field, $2, $1, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | nonrootnode '/' GOTO_PARENT {
            $$ = coda_expression_new(expr_goto_parent, NULL, $1, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | nonrootnode '/' identifier {
            $$ = coda_expression_new(expr_goto_field, $3, $1, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | '[' intexpr ']' {
            $$ = coda_expression_new(expr_goto_array_element, NULL, NULL, $2, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | node '[' intexpr ']' {
            $$ = coda_expression_new(expr_goto_array_element, NULL, $1, $3, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | '@' identifier {
            $$ = coda_expression_new(expr_goto_attribute, $2, NULL, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    | node '@' identifier {
            $$ = coda_expression_new(expr_goto_attribute, $3, $1, NULL, NULL, NULL);
            if ($$ == NULL) YYERROR;
        }
    ;

node:
      rootnode { $$ = $1; }
    | nonrootnode { $$ = $1; }
    ;

%%

/* *INDENT-ON* */

LIBCODA_API int coda_expression_from_string(const char *exprstring, coda_expression **expr)
{
    void *bufstate;

    if (exprstring == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid expression string argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (expr == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid expression argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    coda_errno = 0;
    parsed_expression = NULL;
    bufstate = (void *)coda_expression__scan_string(exprstring);
    if (coda_expression_parse() != 0)
    {
        if (coda_errno == 0)
        {
            coda_set_error(CODA_ERROR_EXPRESSION, NULL);
        }
        coda_expression__delete_buffer(bufstate);
        return -1;
    }
    coda_expression__delete_buffer(bufstate);
    *expr = parsed_expression;

    return 0;
}
