/*
 * Copyright (C) 2007-2015 S[&]T, The Netherlands.
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

%option noyywrap
%option noinput
%option nounput
%option always-interactive

%{

/* *INDENT-ON* */

/* These defines are to allow more than one yacc file in the program */
#define yy_delete_buffer coda_expression_delete_buffer
#define yy_scan_string coda_expression_scan_string
#define yy_yys coda_expression_yys
#define yy_yyv coda_expression_yyv
#define yyact coda_expression_act
#define yychar coda_expression_char
#define yycheck coda_expression_yycheck
#define yychk coda_expression_chk
#define yydebug coda_expression_debug
#define yydef coda_expression_def
#define yydefred coda_expression_yydefred
#define yydgoto coda_expression_yydgoto
#define yyerrflag coda_expression_errflag
#define yyerror coda_expression_error
#define yyexca coda_expression_exca
#define yygindex coda_expression_yygindex
#define yylen coda_expression_yylen
#define yylex coda_expression_lex
#define yylhs coda_expression_yylhs
#define yylloc coda_expression_lloc
#define yylval coda_expression_lval
#define yymaxdepth coda_expression_maxdepth
#define yyname coda_expression_yyname
#define yynerrs coda_expression_nerrs
#define yypact coda_expression_pact
#define yyparse coda_expression_parse
#define yypgo coda_expression_pgo
#define yyps coda_expression_ps
#define yypv coda_expression_pv
#define yyr1 coda_expression_r1
#define yyr2 coda_expression_r2
#define yyreds coda_expression_reds
#define yyrindex coda_expression_yyrindex
#define yyrule coda_expression_yyrule
#define yys coda_expression_s
#define yysindex coda_expression_yysindex
#define yystate coda_expression_state
#define yytable coda_expression_yytable
#define yytmp coda_expression_tmp
#define yytoks coda_expression_toks
#define yyv coda_expression_v
#define yyval coda_expression_val

#include "coda-expr.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "coda-expr-parser.h"

/* *INDENT-OFF* */

%}

INTEGER      [0-9]+
SIGN         [+-]
FL_MANTISSA  ({INTEGER}".")|("."{INTEGER})|({INTEGER}"."{INTEGER})
FL_EXPONENT  [Ee]{SIGN}?{INTEGER}
FLOAT        ({FL_MANTISSA}{FL_EXPONENT}?)|({INTEGER}{FL_EXPONENT})
IDENTIFIER   [[:alpha:]][[:alnum:]_]*
WHITESPACE   [[:space:]]+
INDEX        "i"|"j"|"k"

%%

"\""([^\"\\]|\\.)*"\"" {
                            yylval.stringval = (char *)malloc(yyleng - 1);
                            memcpy(yylval.stringval, yytext + 1, yyleng - 2);
                            yylval.stringval[yyleng - 2] = '\0';
                            return STRING_VALUE;
                        }

">="                    return GREATER_EQUAL;
"<="                    return LESS_EQUAL;
"=="                    return EQUAL;
"!="                    return NOT_EQUAL;
"&"                     return AND;
"|"                     return OR;
"&&"                    return LOGICAL_AND;
"||"                    return LOGICAL_OR;
"!"                     return NOT;
".."                    return GOTO_PARENT;

"r"                     return RAW_PREFIX;
"asciiline"             return ASCIILINE;
"do"                    return DO;
"for"                   return FOR;
"step"                  return STEP;
"to"                    return TO;
"nan"                   return NAN_VALUE;
"inf"                   return INF_VALUE;
"true"                  return TRUE_VALUE;
"false"                 return FALSE_VALUE;
"abs"                   return FUNC_ABS;
"add"                   return FUNC_ADD;
"all"                   return FUNC_ALL;
"bitoffset"             return FUNC_BITOFFSET;
"bitsize"               return FUNC_BITSIZE;
"bool"                  return FUNC_BOOL;
"bytes"                 return FUNC_BYTES;
"byteoffset"            return FUNC_BYTEOFFSET;
"bytesize"              return FUNC_BYTESIZE;
"ceil"                  return FUNC_CEIL;
"count"                 return FUNC_COUNT;
"exists"                return FUNC_EXISTS;
"filename"              return FUNC_FILENAME;
"filesize"              return FUNC_FILESIZE;
"float"                 return FUNC_FLOAT;
"floor"                 return FUNC_FLOOR;
"goto"                  return FUNC_GOTO;
"if"                    return FUNC_IF;
"index"                 return FUNC_INDEX;
"int"                   return FUNC_INT;
"isnan"                 return FUNC_ISNAN;
"isinf"                 return FUNC_ISINF;
"isplusinf"             return FUNC_ISPLUSINF;
"ismininf"              return FUNC_ISMININF;
"length"                return FUNC_LENGTH;
"ltrim"                 return FUNC_LTRIM;
"numelements"           return FUNC_NUMELEMENTS;
"max"                   return FUNC_MAX;
"min"                   return FUNC_MIN;
"productclass"          return FUNC_PRODUCTCLASS;
"productformat"         return FUNC_PRODUCTFORMAT;
"producttype"           return FUNC_PRODUCTTYPE;
"productversion"        return FUNC_PRODUCTVERSION;
"regex"                 return FUNC_REGEX;
"round"                 return FUNC_ROUND;
"rtrim"                 return FUNC_RTRIM;
"str"                   return FUNC_STR;
"strtime"               return FUNC_STRTIME;
"substr"                return FUNC_SUBSTR;
"time"                  return FUNC_TIME;
"trim"                  return FUNC_TRIM;
"unboundindex"          return FUNC_UNBOUNDINDEX;
"with"                  return FUNC_WITH;

{INDEX}                 {
                            yylval.stringval = strdup(yytext);
                            return INDEX_VAR;
                        }
{IDENTIFIER}            {
                            yylval.stringval = strdup(yytext);
                            return NAME;
                        }
{INTEGER}               {
                            yylval.stringval = strdup(yytext);                            
                            return INT_VALUE;
                        }
{FLOAT}                 {
                            yylval.stringval = strdup(yytext);                            
                            return FLOAT_VALUE;
                        }
{WHITESPACE}           /* eat whitespace */
.                      return (*yytext);

%%
