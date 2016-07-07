#pragma once

#define MAX_WORD_LENGTH 0x100
#define MAX_LITERAL_LENGTH 0x100

typedef enum
{
	TT_UNKNOWN = 0,
	TT_WHITESPACE = 1,
	TT_PUNCTUATION = 2,
	TT_WORD = 3,
	TT_KEYWORD = 4,
	TT_INTEGER_LITERAL = 5,
	TT_REAL_LITERAL = 6,
	TT_DATATYPE = 7,
} TokenType;

typedef struct
{
	TokenType type;
	char* value;
} Token;

typedef struct
{
	Token* token;
	struct TokenNode* prev;
	struct TokenNode* next;
} TokenNode;

typedef struct
{
	size_t count;
	TokenNode* first;
	TokenNode* last;
} TokenList;

TokenList* lex(const char* filename);
