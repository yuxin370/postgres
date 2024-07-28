/*-------------------------------------------------------------------------
 *
 * tsvector_parser.c
 *	  Parser for tsvector
 *
 * Portions Copyright (c) 1996-2024, PostgreSQL Global Development Group
 *
 *
 * IDENTIFICATION
 *	  src/backend/utils/adt/tsvector_parser.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "tsearch/ts_locale.h"
#include "tsearch/ts_utils.h"


/*
 * Private state of tsvector parser.  Note that tsquery also uses this code to
 * parse its input, hence the boolean flags.  The oprisdelim and is_tsquery
 * flags are both true or both false in current usage, but we keep them
 * separate for clarity.
 *
 * If oprisdelim is set, the following characters are treated as delimiters
 * (in addition to whitespace): ! | & ( )
 *
 * is_tsquery affects *only* the content of error messages.
 *
 * is_web can be true to further modify tsquery parsing.
 *
 * If escontext is an ErrorSaveContext node, then soft errors can be
 * captured there rather than being thrown.
 */
struct TSVectorParseStateData
{
	char	   *prsbuf;			/* next input character */
	char	   *bufstart;		/* whole string (used only for errors) */
	char	   *word;			/* buffer to hold the current word */
	int			len;			/* size in bytes allocated for 'word' */
	int			eml;			/* max bytes per character */
	bool		oprisdelim;		/* treat ! | * ( ) as delimiters? */
	bool		is_tsquery;		/* say "tsquery" not "tsvector" in errors? */
	bool		is_web;			/* we're in websearch_to_tsquery() */
	Node	   *escontext;		/* for soft error reporting */
};


/*
 * Initializes a parser state object for the given input string.
 * A bitmask of flags (see ts_utils.h) and an error context object
 * can be provided as well.
 */
TSVectorParseState
init_tsvector_parser(char *input, int flags, Node *escontext)
{
	TSVectorParseState state;

	state = (TSVectorParseState) palloc(sizeof(struct TSVectorParseStateData));
	state->prsbuf = input;
	state->bufstart = input;
	state->len = 32;
	state->word = (char *) palloc(state->len);
	state->eml = pg_database_encoding_max_length();
	state->oprisdelim = (flags & P_TSV_OPR_IS_DELIM) != 0;
	state->is_tsquery = (flags & P_TSV_IS_TSQUERY) != 0;
	state->is_web = (flags & P_TSV_IS_WEB) != 0;
	state->escontext = escontext;

	return state;
}

/*
 * Reinitializes parser to parse 'input', instead of previous input.
 *
 * Note that bufstart (the string reported in errors) is not changed.
 */
void
reset_tsvector_parser(TSVectorParseState state, char *input)
{
	state->prsbuf = input;
}

/*
 * Shuts down a tsvector parser.
 */
void
close_tsvector_parser(TSVectorParseState state)
{
	pfree(state->word);
	pfree(state);
}

/* increase the size of 'word' if needed to hold one more character */
#define RESIZEPRSBUF \
do { \
	int clen = curpos - state->word; \
	if ( clen + state->eml >= state->len ) \
	{ \
		state->len *= 2; \
		state->word = (char *) repalloc(state->word, state->len); \
		curpos = state->word + clen; \
	} \
} while (0)

/* Fills gettoken_tsvector's output parameters, and returns true */
#define RETURN_TOKEN \
do { \
	if (pos_ptr != NULL) \
	{ \
		*pos_ptr = pos; \
		*poslen = npos; \
	} \
	else if (pos != NULL) \
		pfree(pos); \
	\
	if (strval != NULL) \
		*strval = state->word; \
	if (lenval != NULL) \
		*lenval = curpos - state->word; \
	if (endptr != NULL) \
		*endptr = state->prsbuf; \
	return true; \
} while(0)


/* State codes used in gettoken_tsvector */
#define WAITWORD		1
#define WAITENDWORD		2
#define WAITNEXTCHAR	3
#define WAITENDCMPLX	4
#define WAITPOSINFO		5
#define INPOSINFO		6
#define WAITPOSDELIM	7
#define WAITCHARCMPLX	8

#define PRSSYNTAXERROR return prssyntaxerror(state)

static bool
prssyntaxerror(TSVectorParseState state)
{
	errsave(state->escontext,
			(errcode(ERRCODE_SYNTAX_ERROR),
			 state->is_tsquery ?
			 errmsg("syntax error in tsquery: \"%s\"", state->bufstart) :
			 errmsg("syntax error in tsvector: \"%s\"", state->bufstart)));
	/* In soft error situation, return false as convenience for caller */
	return false;
}


/*
 * Get next token from string being parsed. Returns true if successful,
 * false if end of input string is reached or soft error.
 *
 * On success, these output parameters are filled in:
 *
 * *strval		pointer to token
 * *lenval		length of *strval
 * *pos_ptr		pointer to a palloc'd array of positions and weights
 *				associated with the token. If the caller is not interested
 *				in the information, NULL can be supplied. Otherwise
 *				the caller is responsible for pfreeing the array.
 * *poslen		number of elements in *pos_ptr
 * *endptr		scan resumption point
 *
 * Pass NULL for any unwanted output parameters.
 *
 * If state->escontext is an ErrorSaveContext, then caller must check
 * SOFT_ERROR_OCCURRED() to determine whether a "false" result means
 * error or normal end-of-string.
 */
bool
gettoken_tsvector(TSVectorParseState state,
				  char **strval, int *lenval,
				  WordEntryPos **pos_ptr, int *poslen,
				  char **endptr)
{
	int			oldstate = 0;
	// 当前正在解析的单词字符串的位置
	char	   *curpos = state->word;
	// 当前解析状态
	int			statecode = WAITWORD;

	/*
	 * pos is for collecting the comma delimited list of positions followed by
	 * the actual token.
	 */
	// 存储位置数组的指针
	WordEntryPos *pos = NULL;
	// 位置数组的实际长度
	int			npos = 0;		/* elements of pos used */
	// 位置数组的分配长度
	int			posalen = 0;	/* allocated size of pos */

	while (1)
	{
		if (statecode == WAITWORD)
		{
			if (*(state->prsbuf) == '\0')
				return false;
			// ' 符号开头意味着后面跟一个复杂函数
			else if (!state->is_web && t_iseq(state->prsbuf, '\''))
				statecode = WAITENDCMPLX;	// 等待复杂单词结束
			// \ 开头意味着后面跟一个转义单词
			else if (!state->is_web && t_iseq(state->prsbuf, '\\'))
			{
				statecode = WAITNEXTCHAR;	// 等待转义单词结束
				oldstate = WAITENDWORD;
			}
			// 如果是一个操作符
			else if ((state->oprisdelim && ISOPERATOR(state->prsbuf)) ||
					 (state->is_web && t_iseq(state->prsbuf, '"')))
				PRSSYNTAXERROR;
			// 如果不是空白字符
			else if (!t_isspace(state->prsbuf))
			{
				COPYCHAR(curpos, state->prsbuf);
				curpos += pg_mblen(state->prsbuf);
				statecode = WAITENDWORD;
			}
		}
		// 等待转义字符后的下一个字符
		else if (statecode == WAITNEXTCHAR)
		{
			if (*(state->prsbuf) == '\0')
				ereturn(state->escontext, false,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("there is no escaped character: \"%s\"",
								state->bufstart)));
			// 抄写一个转义字符，然后恢复原有状态
			else
			{
				RESIZEPRSBUF;
				COPYCHAR(curpos, state->prsbuf);
				curpos += pg_mblen(state->prsbuf);
				Assert(oldstate != 0);
				statecode = oldstate;
			}
		}

		// 等待单词结束
		else if (statecode == WAITENDWORD)
		{
			// 等待转义字符
			if (!state->is_web && t_iseq(state->prsbuf, '\\'))
			{
				statecode = WAITNEXTCHAR;
				oldstate = WAITENDWORD;
			}
			// 单词结束，返回一个 token
			else if (t_isspace(state->prsbuf) || *(state->prsbuf) == '\0' ||
					 (state->oprisdelim && ISOPERATOR(state->prsbuf)) ||
					 (state->is_web && t_iseq(state->prsbuf, '"')))
			{
				RESIZEPRSBUF;
				if (curpos == state->word)
					PRSSYNTAXERROR;
				*(curpos) = '\0';
				RETURN_TOKEN;
			}
			// 进入位置信息解析状态 INPOSINFO
			else if (t_iseq(state->prsbuf, ':'))
			{
				if (curpos == state->word)
					PRSSYNTAXERROR;
				*(curpos) = '\0';
				if (state->oprisdelim)
					RETURN_TOKEN;
				else
					statecode = INPOSINFO;
			}
			else
			{
				RESIZEPRSBUF;
				COPYCHAR(curpos, state->prsbuf);
				curpos += pg_mblen(state->prsbuf);
			}
		}

		// 等待复杂单词结束
		else if (statecode == WAITENDCMPLX)
		{
			if (!state->is_web && t_iseq(state->prsbuf, '\''))
			{
				statecode = WAITCHARCMPLX;
			}
			else if (!state->is_web && t_iseq(state->prsbuf, '\\'))
			{
				statecode = WAITNEXTCHAR;
				oldstate = WAITENDCMPLX;
			}
			else if (*(state->prsbuf) == '\0')
				PRSSYNTAXERROR;
			else
			{
				RESIZEPRSBUF;
				COPYCHAR(curpos, state->prsbuf);
				curpos += pg_mblen(state->prsbuf);
			}
		}

		// 等待复杂字符
		else if (statecode == WAITCHARCMPLX)
		{
			if (!state->is_web && t_iseq(state->prsbuf, '\''))
			{
				RESIZEPRSBUF;
				COPYCHAR(curpos, state->prsbuf);
				curpos += pg_mblen(state->prsbuf);
				statecode = WAITENDCMPLX;
			}
			else
			{
				RESIZEPRSBUF;
				*(curpos) = '\0';
				if (curpos == state->word)
					PRSSYNTAXERROR;
				if (state->oprisdelim)
				{
					/* state->prsbuf+=pg_mblen(state->prsbuf); */
					RETURN_TOKEN;
				}
				else
					statecode = WAITPOSINFO;
				continue;		/* recheck current character */
			}
		}

		// 等待位置信息开始
		else if (statecode == WAITPOSINFO)
		{
			if (t_iseq(state->prsbuf, ':'))
				statecode = INPOSINFO;
			else
				RETURN_TOKEN;
		}

		// 解析位置信息中
		else if (statecode == INPOSINFO)
		{
			if (t_isdigit(state->prsbuf))
			{
				if (posalen == 0)
				{
					posalen = 4;
					pos = (WordEntryPos *) palloc(sizeof(WordEntryPos) * posalen);
					npos = 0;
				}
				else if (npos + 1 >= posalen)
				{
					posalen *= 2;
					pos = (WordEntryPos *) repalloc(pos, sizeof(WordEntryPos) * posalen);
				}
				npos++;
				WEP_SETPOS(pos[npos - 1], LIMITPOS(atoi(state->prsbuf)));
				/* we cannot get here in tsquery, so no need for 2 errmsgs */
				if (WEP_GETPOS(pos[npos - 1]) == 0)
					ereturn(state->escontext, false,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg("wrong position info in tsvector: \"%s\"",
									state->bufstart)));
				WEP_SETWEIGHT(pos[npos - 1], 0);
				statecode = WAITPOSDELIM;
			}
			else
				PRSSYNTAXERROR;
		}

		// 等待位置分隔符
		else if (statecode == WAITPOSDELIM)
		{
			if (t_iseq(state->prsbuf, ','))
				statecode = INPOSINFO;
			else if (t_iseq(state->prsbuf, 'a') || t_iseq(state->prsbuf, 'A') || t_iseq(state->prsbuf, '*'))
			{
				if (WEP_GETWEIGHT(pos[npos - 1]))
					PRSSYNTAXERROR;
				WEP_SETWEIGHT(pos[npos - 1], 3);
			}
			else if (t_iseq(state->prsbuf, 'b') || t_iseq(state->prsbuf, 'B'))
			{
				if (WEP_GETWEIGHT(pos[npos - 1]))
					PRSSYNTAXERROR;
				WEP_SETWEIGHT(pos[npos - 1], 2);
			}
			else if (t_iseq(state->prsbuf, 'c') || t_iseq(state->prsbuf, 'C'))
			{
				if (WEP_GETWEIGHT(pos[npos - 1]))
					PRSSYNTAXERROR;
				WEP_SETWEIGHT(pos[npos - 1], 1);
			}
			else if (t_iseq(state->prsbuf, 'd') || t_iseq(state->prsbuf, 'D'))
			{
				if (WEP_GETWEIGHT(pos[npos - 1]))
					PRSSYNTAXERROR;
				WEP_SETWEIGHT(pos[npos - 1], 0);
			}
			else if (t_isspace(state->prsbuf) ||
					 *(state->prsbuf) == '\0')
				RETURN_TOKEN;
			else if (!t_isdigit(state->prsbuf))
				PRSSYNTAXERROR;
		}
		else					/* internal error */
			elog(ERROR, "unrecognized state in gettoken_tsvector: %d",
				 statecode);

		/* get next char */
		state->prsbuf += pg_mblen(state->prsbuf);
	}
}
