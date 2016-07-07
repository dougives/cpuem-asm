#define _CRT_SECURE_NO_WARNINGS

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "lex.h"

const char* keywords[] = 
{
	"halt",
	/*
	"byte",
	"sbyte",
	"ushort",
	"short",
	"uint",
	"int",
	"ulong",
	"long",
	"ulong",
	"function",
	"node",
	"list",
	*/
};

const char* datatypes[] =
{
	"byte",
	"sbyte",
	"ushort",
	"short",
	"uint",
	"int",
	"ulong",
	"long",
	"single",
	"double",
	"function",
	"node",
	"list",
};

static void lexing_error(const char* msg)
{
	printf("%s\n", msg);
	getchar();
	exit(1);
}

static void add_token_to_list(TokenList* list, Token* token)
{
	TokenNode* node = (TokenNode*)malloc(sizeof(TokenNode));
	if (node == NULL)
		lexing_error("could not add token to list.");
	node->next = NULL;
	node->token = token;

	if (list->count == 0)
	{
		list->first = node;
		list->last = node;
		list->count = 1;
		return;
	}
	node->prev = (struct TokenNode*)list->last;
	list->last->next = (struct TokenNode*)node;
	list->last = node;
	list->count++;
	return;
}

static Token* create_token(TokenType type, char* value)
{
	Token* token = (Token*)malloc(sizeof(Token));
	if (token == NULL)
		lexing_error("could not create token.");
	token->type = type;
	token->value = value;
	return token;
}

static bool is_decimal_digit(char c)
{
	return c > 0x2f && c < 0x3a;
}

static bool is_number_literal_part(char c, bool ishex)
{
	char lower = c | 0x20;
	return
		is_decimal_digit(c)
		| (ishex
			&& (lower > 0x60 && lower < 0x67)) 
		| (c == '.')
		| (lower == 'e');
}

static bool is_number_literal(TokenList* list, char c, FILE* file)
{
	if (!(is_decimal_digit(c)
		|| c != '-'
		|| c != '.'))
		return false;
	char buffer[MAX_LITERAL_LENGTH];
	size_t count = 0;
	buffer[count++] = c;
	bool isreal = (c == '.');
	bool ishex = false;
	if (c == '0')
	{
		c = fgetc(file);
		ishex = ((c | 0x20) == 'x');
		buffer[count++] = c;
	}

	for (
		;// count;
		is_number_literal_part(c = fgetc(file), ishex)
		&& count < MAX_LITERAL_LENGTH;
		count++)
	{
		if (c == '.')
		{
			if (isreal)
				lexing_error("malformed number literal.");
			isreal = true;
			if (ishex && isreal)
				lexing_error("malformed number literal.");
		}
		buffer[count] = c;
	}
	ungetc(c, file);
	if (count >= MAX_LITERAL_LENGTH)
		lexing_error("literal was longer than max literal length.");
	buffer[count++] = '\0';
	char* value = (char*)malloc(count);
	if (value == NULL)
		lexing_error("could not allocate space for literal string value.");
	memcpy(value, buffer, count);
	add_token_to_list(
		list,
		create_token(
			isreal ? TT_REAL_LITERAL : TT_INTEGER_LITERAL, 
			value));
	return true;
}

static bool is_literal(TokenList* list, char c, FILE* file)
{
	return
		  is_number_literal(list, c, file);
}

static bool is_letter(char c)
{
	c |= 0x20;
	return c > 0x60 && c < 0x7b;
}

static bool is_keyword(const char* word, size_t size)
{
	for (
		size_t i = 0;
		i < sizeof(keywords) / sizeof(char*);
		i++)
		if (strncmp(word, keywords[i], size) == 0)
			return true;
	return false;
}

static bool is_datatype(const char* word, size_t size)
{
	for (
		size_t i = 0;
		i < sizeof(datatypes) / sizeof(char*);
		i++)
		if (strncmp(word, datatypes[i], size) == 0)
			return true;
	return false;
}

static bool is_wordpart(char c)
{
	return is_letter(c) || is_decimal_digit(c);
}

static bool is_word(TokenList* list, char c, FILE* file)
{
	if (!(is_letter(c)
		//| is_decimal_digit(c)
		|| (c == '_')))
		return false;
	ungetc(c, file);

	char buffer[MAX_WORD_LENGTH];
	size_t count;
	for (
		count = 0;
		is_wordpart(c = fgetc(file))
		&& count < MAX_WORD_LENGTH;
		//&& c != EOF;
		count++)
	{
		buffer[count] = c;
	}
	ungetc(c, file);
	if (count >= MAX_WORD_LENGTH)
		lexing_error("word was longer than max word length.");
	buffer[count++] = '\0';
	char* value = (char*)malloc(count);
	if (value == NULL)
		lexing_error("could not allocate space for word string value.");
	memcpy(value, buffer, count);
	add_token_to_list(
		list,
		create_token(
			is_keyword(value, count) 
				? TT_KEYWORD 
			: is_datatype(value, count)
				? TT_DATATYPE
			: TT_WORD, 
			value));
	return true;
}

static bool is_punctuation(TokenList* list, char c)
{
	if (!((c == '.')
		| (c == ':')
		| (c == '[')
		| (c == ']')
		| (c == '{')
		| (c == '}')
		| (c == '(')
		| (c == ')')))
		return false;

	char* copy = (char*)malloc(sizeof(char) * 2); // ...
	*copy = c;
	*(copy + 1) = '\0';
	add_token_to_list(
		list,
		create_token(TT_PUNCTUATION, copy));
	return true;
}

static bool is_whitespace(char c)
{
	return
		  (c == ' '	)
		| (c == '\n')
		| (c == '\r')
		| (c == '\t')
		| (c == '\v')
		| (c == '\f');
}

TokenList* lex(const char* filename)
{
	TokenList* list =
		(TokenList*)malloc(sizeof(TokenList));
	if (list == NULL)
		return NULL;
	list->count = 0;
	list->first = NULL;
	list->last = NULL;

	FILE* file = fopen(filename, "r");
	if (file == NULL)
		return NULL;

	char c;
	while ((c = (char)fgetc(file)) != EOF)
	{
		if (is_whitespace(c))
			continue;
		if (is_punctuation(list, c))
			continue;
		if (is_word(list, c, file))
			continue;
		if (is_literal(list, c, file))
			continue;
		lexing_error("unknown token.");
	}

	return list;
}






