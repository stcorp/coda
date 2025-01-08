#ifndef CODA_PCRE2_CONFIG_H
#define CODA_PCRE2_CONFIG_H

#define PCRE2_EXPORT
#define PCRE2_CODE_UNIT_WIDTH 8

#define HEAP_LIMIT 20000000
#define LINK_SIZE 2
#define MATCH_LIMIT 10000000
#define MATCH_LIMIT_DEPTH MATCH_LIMIT
#define MAX_NAME_COUNT 10000
#define MAX_NAME_SIZE 32
#define MAX_VARLOOKBEHIND 255
#define NEWLINE_DEFAULT 2
#define PARENS_NEST_LIMIT 250
#define PCRE2GREP_BUFSIZE 20480
#define PCRE2GREP_MAX_BUFSIZE 1048576
#define PCRE2_STATIC

#endif
