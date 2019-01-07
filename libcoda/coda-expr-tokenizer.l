/*
 * Copyright (C) 2007-2019 S[&]T, The Netherlands.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* *INDENT-OFF* */

%option prefix="coda_expression_"
%option noyywrap
%option noinput
%option nounput
%option always-interactive
%option reentrant
%option bison-bridge

%{
#include "coda-expr.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "coda-expr-parser.h"
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

"\""([^\"\\]|\\.)*"\""  {
                            yylval->stringval = (char *)malloc(yyleng - 1);
                            memcpy(yylval->stringval, yytext + 1, yyleng - 2);
                            yylval->stringval[yyleng - 2] = '\0';
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
"at"                    return FUNC_AT;
"bitoffset"             return FUNC_BITOFFSET;
"bitsize"               return FUNC_BITSIZE;
"bool"                  return FUNC_BOOL;
"bytes"                 return FUNC_BYTES;
"byteoffset"            return FUNC_BYTEOFFSET;
"bytesize"              return FUNC_BYTESIZE;
"ceil"                  return FUNC_CEIL;
"count"                 return FUNC_COUNT;
"dim"                   return FUNC_DIM;
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
"numdims"               return FUNC_NUMDIMS;
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
                            yylval->stringval = strdup(yytext);
                            return INDEX_VAR;
                        }
{IDENTIFIER}            {
                            yylval->stringval = strdup(yytext);
                            return NAME;
                        }
{INTEGER}               {
                            yylval->stringval = strdup(yytext);
                            return INT_VALUE;
                        }
{FLOAT}                 {
                            yylval->stringval = strdup(yytext);
                            return FLOAT_VALUE;
                        }
{WHITESPACE}            /* eat whitespace */
.                       return (*yytext);

%%
