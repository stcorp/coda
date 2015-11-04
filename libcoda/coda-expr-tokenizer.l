/*
 * Copyright (C) 2007-2012 S[&]T, The Netherlands.
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

%option outfile="lex.yy.c"
%option prefix="coda_expression_"
%option noyywrap
%option nounput
%option always-interactive

%{

/* *INDENT-ON* */

/* This define is to allow more than one yacc file in the program */
#define yylval coda_expression_lval

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
"substr"                return FUNC_SUBSTR;
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
