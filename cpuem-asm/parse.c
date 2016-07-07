#define _CRT_SECURE_NO_WARNINGS


#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "lex.h"

#define HASH_ALLZEROS ((uint8_t[]){  0uL,  0uL,  0uL,  0uL, })
#define HASH_ALLONES  ((uint8_t[]){ ~0uL, ~0uL, ~0uL, ~0uL, })

#define INVALID_OPERATION_SELECTION 0xff

typedef enum
{
	DT_BYTE		= 0x0,
	DT_SBYTE	= 0x4,
	DT_USHORT	= 0x1,
	DT_SHORT	= 0x5,
	DT_UINT		= 0x2,
	DT_INT		= 0x6,
	DT_ULONG	= 0x3,
	DT_LONG		= 0x7,
	DT_HALF		= 0x8,
	DT_SINGLE	= 0x9,
	DT_DOUBLE	= 0xa,
	DT_QUAD		= 0xb,
	DT_FUNCTION	= 0xc,
	DT_NODE		= 0xd,
	DT_LIST		= 0xe,
	DT_RESERVED	= 0xf,
} DataType;

const char* typemap[] =
{
	"byte",
	"ushort",
	"uint",
	"ulong",
	"sbyte",
	"short",
	"int",
	"long",
	"half",
	"single",
	"double",
	"quad",
	"function",
	"node",
	"list",
};

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
		float		reals;
		double		reald;
		struct Function*	func;
		struct Node*		node;
		struct List*		list;
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
	const char* ops[0x10];
} OpMap;

typedef struct
{
	OpMap opmaps[0x10];
} OpMapTable;

typedef struct
{
	const char* text;
	bool hasimm;
	DataType type;
	OpMap* opmap;
	uint8_t selection;
	Data imm;
} OperationSymbol;

typedef struct
{
	bool gt, lt, z, eq;
	const char* label;
	bool resolved;
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
		struct FnCallSymbol* fncall;
	};
} Symbol;

typedef struct
{
	const char* identifier;
	Hash hash;
	bool isloaded;
	Symbol* block;
	size_t count;
} Function;

typedef struct
{
	Function* func;
	void* args;
	size_t count;
} FnCallSymbol;

typedef struct
{
	DataType settype;
} ParserState;

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

Symbol parse_operation(TokenNode* node, ParserState state)
{
	Symbol symbol;

	OperationSymbol* operation = 
		(OperationSymbol*)malloc(sizeof(OperationSymbol));

	char* typetext = NULL;
	if (node->token->type == TT_DATATYPE)
	{
		typetext = node->token->value;
		int mapindex;
		for (
			mapindex = 0;
			mapindex < sizeof(typemap) / sizeof(char*);
			mapindex++)
			if (
				strncmp(
					node->token->value,
					typemap[mapindex],
					MAX_WORD_LENGTH)
				== 0)
				break;

		state.settype = (DataType)mapindex;

		node = (TokenNode*)node->next;
		if (node == NULL)
			out_of_tokens_error();
	}

	operation->type = state.settype;
	operation->hasimm = false;
	operation->opmap = NULL;
	operation->selection = INVALID_OPERATION_SELECTION;
	memset(&operation->imm, 0, sizeof(Data));

	char* endptr = NULL;
	char* littext = NULL;
	if (node->token->type == TT_INTEGER_LITERAL
		|| node->token->type == TT_REAL_LITERAL)
	{
		littext = node->token->value;
		operation->hasimm = true;
		Data data;
		data.type = state.settype;
		switch (node->token->type)
		{
		case TT_REAL_LITERAL:
			switch (state.settype)
			{
			case DT_SINGLE:
				data.reals = strtof(node->token->value, &endptr);
				break;
			case DT_DOUBLE:
				data.reald = strtod(node->token->value, &endptr);
				break;
			default:
				parsing_error("expected real datatype to be set.");
			}
			break;
		case TT_INTEGER_LITERAL:
			data.L = strtoll(node->token->value, &endptr, 0);
			break;
		}
		operation->imm = data;

		node = (TokenNode*)node->next;
		if (node == NULL)
			out_of_tokens_error();
	}

	operation->text =
		(node->token->type == TT_WORD)
		? node->token->value
		: "nop";

	symbol.type = ST_OPERATION;
	symbol.operation = operation;
	size_t symtextlength = 0;
	size_t typetextlength = 0;
	size_t littextlength = 0;
	size_t optextlength = 0;
	symtextlength +=
		(typetext != NULL)
		? typetextlength = strnlen(typetext, MAX_WORD_LENGTH) + 1
		: 0;
	symtextlength +=
		(littext != NULL)
		? littextlength = strnlen(littext, MAX_LITERAL_LENGTH) + 1
		: 0;
	symtextlength +=
		(optextlength =
			strnlen(
				operation->text,
				MAX_WORD_LENGTH)
			+ 1);
	char* symtext = (char*)malloc(symtextlength);
	memcpy(
		symtext, 
		typetext, 
		typetextlength - 1);
	symtext[typetextlength] = ' ';
	memcpy(
		symtext + typetextlength, 
		littext, 
		littextlength - 1);
	symtext[littextlength + typetextlength] = ' ';
	memcpy(
		symtext + littextlength + typetextlength, 
		operation->text, 
		optextlength - 1);
	symtext[symtextlength - 1] = '\0';
	symbol.text = (const char*)symtext;
	return symbol;
}

Symbol parse_fncall(TokenNode* node)
{
	Symbol symbol;

	// create halt function call symbol
	FnCallSymbol* fncall =
		(FnCallSymbol*)malloc(sizeof(FnCallSymbol));
	Function* func =
		(Function*)malloc(sizeof(Function));
	if (fncall == NULL
		|| func == NULL)
		parsing_error("could not allocate function for fncall symbol.");
	func->identifier = node->token->value;
	memcpy(func->hash.value, HASH_ALLZEROS, sizeof(func->hash.value));
	func->isloaded = false;
	func->block = NULL;
	func->count = 0;
	fncall->func = func;
	fncall->args = NULL;

	node = (TokenNode*)node->next;
	if (node == NULL)
		out_of_tokens_error();
	if (!(node->token->type == TT_PUNCTUATION
		&& *node->token->value == '('))
		goto malformed_fncall;

	node = (TokenNode*)node->next;
	if (node == NULL)
		out_of_tokens_error();

	if (node->token->type != TT_INTEGER_LITERAL)
		goto malformed_fncall;
	char* endptr = NULL;
	fncall->count = strtoul(node->token->value, &endptr, 0);
	if (endptr == NULL)
		goto malformed_fncall;

	node = (TokenNode*)node->next;
	if (node == NULL)
		out_of_tokens_error();
	if (!(node->token->type == TT_PUNCTUATION
		&& *node->token->value == ')'))
		goto malformed_fncall;

	symbol.type = ST_FNCALL;
	symbol.fncall = (struct FnCallSymbol*)fncall;
	return symbol;

malformed_fncall:
	parsing_error("malformed fncall.");
	//return symbol;
}

Symbol parse_branch(TokenNode* node)
{
	char* branchtext = node->token->value;
	size_t textlength = strnlen(branchtext, 6);

	Symbol symbol;

	if (strncmp(branchtext, "br", 2) != 0)
		goto malformed_branch;

	BranchSymbol* branch = 
		(BranchSymbol*)malloc(sizeof(BranchSymbol));
	if (branch == NULL)
		parsing_error("could not allocate memory for branch symbol.");
	size_t position = 2;
	branch->lt = (strncmp(branchtext + position, "lt", 2) == 0);
	branch->gt = (strncmp(branchtext + position, "gt", 2) == 0);
	if (branch->lt || branch->gt)
		position += 2;
	if (strncmp(branchtext + position, "neq", 3) == 0)
	{
		if (branch->lt && branch->gt)
			goto malformed_branch;
		branch->lt = branch->gt = true;
		position += 3;
	}
	branch->eq = (strncmp(branchtext + position, "eq", 2) == 0);
	if (branch->eq 
		&& branch->lt 
		&& branch->gt)
		goto malformed_branch;
	branch->z = (branchtext[position] == 'z');
	
	node = (TokenNode*)node->next;
	if (node == NULL)
		out_of_tokens_error();
	if (!(node->token->type == TT_PUNCTUATION
		&& *node->token->value == '.'))
		goto malformed_branch;

	node = (TokenNode*)node->next;
	if (node == NULL)
		out_of_tokens_error();

	if (node->token->type != TT_WORD)
		goto malformed_branch;

	branch->resolved = false;
	branch->label = node->token->value;
	
	size_t labellength = strnlen(branch->label, MAX_WORD_LENGTH);
	char* symtext =
		(char*)malloc(
			textlength
			+ labellength
			+ 2);
	memcpy(symtext, branchtext, textlength);
	symtext[textlength] = '.';
	memcpy(symtext + textlength + 1, branch->label, labellength);
	symtext[textlength + labellength + 1] = '\0';
	symbol.text = (const char*)symtext;
	symbol.type = ST_BRANCH;
	symbol.branch = branch;

	return symbol;

malformed_branch:
	parsing_error("malformed branch.");
	//return symbol;
}

Symbol parse_word(TokenNode* node, ParserState state)
{
	TokenNode* next = (TokenNode*)node->next;
	if (next == NULL)
		out_of_tokens_error();
	if (next->token->type == TT_PUNCTUATION)
	{
		if (*next->token->value == '.')
			return parse_branch(node);
		if (*next->token->value == '(')
			return parse_fncall(node);
		parsing_error("unexpected punctuation after word");
	}

	// must be an operation ...
	return parse_operation(node, state);
}

Symbol parse_keyword(TokenNode* node, ParserState state)
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
		symbol.fncall = (struct FnCallSymbol*)fncall;
		return symbol;
	}

	parsing_error("unknown keyword.");
	return symbol;
}

Symbol* parse_function_block(TokenNode* node, ParserState state, size_t* count)
{
	if (node == NULL)
		out_of_tokens_error();

	if (node->token->type != TT_PUNCTUATION
		|| *node->token->value != '{')
		parsing_error("expected '{' at beginning of function block.");

	Symbol* symbols = NULL;
	*count = 0;
	while (true)
	{
		node = (TokenNode*)node->next;
		if (node == NULL)
			out_of_tokens_error();

		if (node->token->type == TT_PUNCTUATION
			&& *node->token->value == '}')
			return symbols;

		symbols = realloc(symbols, sizeof(Symbol) * (++*count));

		switch (node->token->type)
		{
		case TT_KEYWORD:
			symbols[*count - 1] = parse_keyword(node, state);
			continue;
		case TT_WORD:
			symbols[*count - 1] = parse_word(node, state);
			continue;
			/*
		case TT_INTEGER_LITERAL:
			symbols[count - 1] = parse_integer_literal(node);
			continue;
		case TT_REAL_LITERAL:
			symbols[count - 1] = parse_real_literal(node);
			continue;
			*/
		default:
			parsing_error("unexpected token in function block.");
		}
	}
}

Function* parse_function(TokenNode* node, ParserState state)
{
	if (node == NULL)
		parsing_error("unexpected end of token list.");

	Token* token = node->token;
	if (token->type != TT_WORD)
		parsing_error("expected function identifier.");
	Function* func = (Function*)malloc(sizeof(Function));
	if (func == NULL)
		parsing_error("could not allocate memory for function.");
	func->identifier = token->value;
	func->block = 
		parse_function_block(
		(TokenNode*)node->next, 
			state, 
			&func->count);
	func->isloaded = false;
	memcpy(func->hash.value, HASH_ALLZEROS, sizeof(func->hash.value));
	
	return func;
}

void parse(const char* infilename, const char* outfilename)
{
	ParserState state;
	state.settype = DT_INT;

	FILE* outfile = fopen(outfilename, "r+");
	TokenList* list = lex(infilename);
	if (list == NULL)
		parsing_error("could not lex input file.");
	TokenNode* node = list->first;
	while (node != NULL)
	{
		parse_function(node, state);
	}
}


int main(void)
{
	parse("test.asm", "test.o");
	return 0;
}