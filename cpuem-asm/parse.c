#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "lex.h"

#define HASH_ALLONES ((uint8_t[]){ ~0uL, ~0uL, ~0uL, ~0uL, })

typedef enum
{
	DT_BYTE		= 0x0,
	DT_SBYTE	= 0x4,
	DT_USHORT	= 0x1,
	DT_SHORT	= 0x5,
	DT_UINT		= 0x2,
	DT_INT		= 0x6,
	DT_LONG		= 0x3,
	DT_ULONG	= 0x7,
	DT_HALF		= 0x8,
	DT_SINGLE	= 0x9,
	DT_DOUBLE	= 0xa,
	DT_QUAD		= 0xb,
	DT_FUNCTION	= 0xc,
	DT_NODE		= 0xd,
	DT_LIST		= 0xe,
	DT_RESERVED	= 0xf,
} DataType;

typedef struct
{
	DataType type;
	union
	{
		uint8_t		b;
		int8_t		sb;
		uint16_t	us;
		int16_t		s;
		uint32_t	ui;
		int32_t		i;
		uint64_t	uL;
		int64_t		L;
		Function*	func;
		Node*		node;
		List*		list;
	};
} Data;

typedef struct
{
	uint8_t value[32];
} Hash;

typedef struct
{
	Hash hash;
} Node;

typedef struct
{
	Data data;
	struct List* next;
} List;

typedef enum
{
	ST_UNKNOWN = 0,
	ST_BRANCH = 1,
	ST_OPERATION = 2,
	ST_FNCALL = 3,
} SymbolType;

typedef struct
{
	Function* func;
	void* args;
	size_t count;
} FnCallSymbol;

typedef struct
{
	const char* ops[0x10];
} OpMap;

typedef struct
{
	OpMap opmaps[0x10];
} OpMapTable;

typedef struct
{
	bool hasimm;
	DataType type;
	OpMap* opmap;
	uint8_t selection;
	Data imm;
} OperationSymbol;

typedef struct
{
	bool gt, lt, z, eq;
	int offset;
} BranchSymbol;

typedef struct
{
	SymbolType type;
	const char* text;
	union SymbolData
	{
		BranchSymbol* branch;
		OperationSymbol* operation;
		FnCallSymbol* fncall;
	};
} Symbol;

typedef struct
{
	Symbol* identifier;
	Hash hash;
	bool isloaded;
	Symbol* block;
	size_t count;
} Function;

static void parsing_error(const char* msg)
{
	printf("%s\n", msg);
	getchar();
	exit(1);
}

static void out_of_tokens_error()
{
	parsing_error("unexpected end of token list.");
}

Symbol parse_word(TokenNode* node)
{
	Symbol symbol;
	
}

Symbol parse_keyword(TokenNode* node)
{
	Symbol symbol;
	symbol.text = node->token->value;

	if (strcmp(symbol.text, "halt") == 0)
	{
		// create halt function call symbol
		FnCallSymbol* fncall = 
			(FnCallSymbol*)malloc(sizeof(FnCallSymbol));
		Function* func =
			(Function*)malloc(sizeof(Function));
		if (fncall == NULL
			|| func == NULL)
			parsing_error("could not allocate halt function symbol.");
		func->identifier = "halt";
		memcpy(func->hash.value, HASH_ALLONES, sizeof(func->hash.value));
		func->isloaded = false;
		func->block = NULL;
		func->count = 0;
		fncall->func = func;
		fncall->args = NULL;
		fncall->count = 0;
		symbol.type = ST_FNCALL;
		symbol.fncall = fncall;
		return symbol;
	}

	parsing_error("unknown keyword.");
	return symbol;
}

Symbol* parse_function_block(TokenNode* node)
{
	if (node == NULL)
		out_of_tokens_error();

	if (node->token->type != TT_PUNCTUATION
		|| *node->token->value != '{')
		parsing_error("expected '{' at beginning of function block.");

	Symbol* symbols = NULL;
	size_t count = 0;
	while (true)
	{
		node = node->next;
		if (node == NULL)
			out_of_tokens_error();

		if (node->token->type == TT_PUNCTUATION
			&& *node->token->value == '}')
			return symbols;

		symbols = realloc(symbols, sizeof(Symbol) * (++count));

		switch (node->token->type)
		{
		case TT_KEYWORD:
			symbols[count - 1] = parse_keyword(node);
			continue;
		case TT_WORD:
			symbols[count - 1] = parse_word(node);
			continue;
		case TT_INTEGER_LITERAL:
			symbols[count - 1] = parse_integer_literal(node);
			continue;
		case TT_REAL_LITERAL:
			symbols[count - 1] = parse_real_literal(node);
			continue;
		default:
			parsing_error("unexpected token in function block.");
		}
	}
}

void parse_function(TokenNode* node, FILE* outfile)
{
	if (node == NULL)
		parsing_error("unexpected end of token list.");

	Token* token = node->token;
	if (token != TT_WORD)
		parsing_error("expected function identifier.");
	Function* func = (Function*)malloc(sizeof(Function));
	if (func == NULL)
		parsing_error("could not allocate memory for function.");
	func->identifier = token->value;
	func->block = parse_function_block(node->next);
}

void parse(const char* infilename, const char* outfilename)
{
	FILE* outfile = fopen(outfilename, "r+");
	TokenList* list = lex(infilename);
	if (list == NULL)
		parsing_error("could not lex input file.");
	TokenNode* node = list->first;
	while (node != NULL)
	{
		//parse_function(tokennode);
	}
}