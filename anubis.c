/***************************************************************************
 * anubis.c
 * Character-based parser for Anubis language
 **************************************************************************/
 
/* INCLUDE FILES */
#include "general.h"

#include <string.h>
#include <ctype.h>

#include "parse.h"
#include "read.h"
#include "vstring.h"

/* DATA DEFINITIONS */
typedef enum eAnubisKinds {
    K_FUNCTION,
    K_TYPE,
    K_TYPE_ALTERNATIVE
} anubisKind;

static kindOption AnubisKinds [] = {
    { TRUE, 'f', "function", "function definitions" },
    { TRUE, 't', "typedef", "type definitions" },
    { TRUE, 'a', "macro", "alternative" }
};

static int nextChar (void)
{
	int c = fileGetc ();
	if (c == '\\')
	{
		c = fileGetc ();
		if (c == '\n')
			c = fileGetc ();
	}
	return c;
}

static void skipToMatch (const char *const pair)
{
	const int begin = pair [0], end = pair [1];
	const unsigned long inputLineNumber = getInputLineNumber ();
	int matchLevel = 1;
	int c = '\0';

	while (matchLevel > 0)
	{
		c = nextChar ();
		
		if (c == begin)
			++matchLevel;
		else if (c == end)
			--matchLevel;
		else if (c == '\n')
			break;
	}
	if (c == EOF)
		verbose ("%s: failed to find match for '%c' at line %lu\n",
				getInputFileName (), begin, inputLineNumber);
}

static boolean isFunctionName (int c)
{
	return (boolean)(c != '\0' && ((c >= 65 && c <= 90) || (c >= 97 && c <= 122) || (c >= 48 && c <= 57) || c == 95));
}

static boolean isTypeName (int c)
{
	return (boolean)(c != '\0' && ((c >= 65 && c <= 90) || (c >= 97 && c <= 122) || (c >= 48 && c <= 57) || c == 95));
}

static boolean isAlternativeName (int c)
{
	return (boolean)(c != '\0' && ((c >= 65 && c <= 90) || (c >= 97 && c <= 122) || (c >= 48 && c <= 57) || c == 95));
}

static boolean isKeyword (int c)
{
	return (boolean)(c != '\0' && ((c >= 65 && c <= 90) || (c >= 97 && c <= 122)));
}

static boolean isIdentifier (int c)
{
	return (boolean)(c != '\0' && strchr (".-_ ", c) != NULL);
}

static void readSomething (const int first, vString *const id, boolean (*fn)(int c))
{
	int c = first;
	vStringClear (id);
	while (fn (c))
	{
		vStringPut (id, c);
		c = nextChar ();
	}
	fileUngetc (c);
	vStringTerminate (id);
}

static int skipToNonWhite (void)
{
	int c;
	do {
		c = nextChar ();
	} while (c != '\n' && (isspace (c) || c == '\t'));
	
	return c;
}

static int skipNewlineToNextChar (const int first)
{
	int c = first;
	while (c == '\n') {
		c = nextChar ();
		
		if (c == ' ') {
			c = skipToNonWhite ();
		}
	};
	
	return c;
}

static int skipLineComment (void)
{
	int c;
	
	skipNewlineWhitespaceComment:
	c = skipToNonWhite ();
	c = skipNewlineToNextChar (c);

	if (c == '/')
	{
		if ((c = nextChar ()) == '/')
		{
			do
			{
				c = nextChar ();
			} while (c != '\n' && c != EOF);
		
			if (c == '\n')
			{
				goto skipNewlineWhitespaceComment;
			}
		}
		else
		{
			c = skipToNonWhite ();
			c = skipNewlineToNextChar (c);
		}
	}
	
	return c;
}

static boolean readType (const int first, vString *const id)
{
	int c = first;
	while (!isspace (c))
	{
		c = nextChar ();
	}

	c = skipLineComment();

	if (c >= 65 && c <= 90)
	{
		readSomething(c, id, (void *)isTypeName);
			
		makeSimpleTag (id, AnubisKinds, K_TYPE);
		
		c = skipToNonWhite ();
		c = skipNewlineToNextChar(c);
		
		if (c == '(')
		{
			skipToMatch ("()");
			c = fileGetc ();
		}

		if (c == ':')
		{
			// check alternatives
			while (c != EOF)
			{
				c = skipLineComment();

				if (c >= 97 && c <= 122) {
					readSomething(c, id, (void *)isAlternativeName);
					
					makeSimpleTag (id, AnubisKinds, K_TYPE_ALTERNATIVE);
					
					while (c != ',' && c != '.' && c != EOF)
					{
						c = nextChar ();
						
						if (c == '(')
						{
							skipToMatch ("()");
							c = fileGetc ();
						}
					}

					if (c == EOF)
					{
						break;
					}
					else
					{
						if (c == '.')
						{
							break;
						}
					}
				}
				else
				{
					break;
				}
			}
		}
		
		return TRUE;
	}
	
	return FALSE;
}

static int skipKnownIdentifier (const int first)
{
	int c = first;
	while (!isspace (c))
	{
		c = nextChar ();
	}
	
	c = skipLineComment();
	
	return c;
}

/* FUNCTION DEFINITIONS */

static void findAnubisTags (void)
{
    vString *name = vStringNew ();
    boolean newline = TRUE;
    boolean inpar = FALSE;
    int c;

    while ((c = nextChar ()) != EOF)
    {
		if (newline)
		{
			if (c >= 97 && c <= 122)
			{
				readSomething(c, name, (void *)isKeyword);
			
				if (strcmp (vStringValue (name), "global") == 0 ||
					strcmp (vStringValue (name), "public") == 0 ||
					strcmp (vStringValue (name), "define") == 0 ||
					strcmp (vStringValue (name), "Global") == 0 ||
					strcmp (vStringValue (name), "Public") == 0 ||
					strcmp (vStringValue (name), "Define") == 0)
				{
					c = skipLineComment();
					
					readSomething(c, name, (void *)isKeyword);

					if (strcmp (vStringValue (name), "macro")  == 0 ||
						strcmp (vStringValue (name), "inline") == 0)
					{
						c = skipKnownIdentifier(c);
					}
					else if (strcmp (vStringValue (name), "define") == 0)
					{
						c = skipKnownIdentifier(c);
						
						readSomething(c, name, (void *)isKeyword);
						
						if (strcmp (vStringValue (name), "macro")  == 0 ||
							strcmp (vStringValue (name), "inline") == 0)
						{
							while (!isspace (c))
							{
								c = nextChar ();
							}
						
							c = skipToNonWhite ();
						}
						else if (strcmp (vStringValue (name), "type")  == 0)
						{
							if (readType(c, name)) {
								continue;
							}
						}
					}
					else if (strcmp (vStringValue (name), "type") == 0)
					{
						if (readType(c, name)) {
							continue;
						}
					}
					
					// function return type
					if (c >= 65 && c <= 90 || c == '(')
					{
						skipType:
						if (c == '(')
						{
							skipToMatch ("()");
						}
						
						while (!isspace (c))
						{
							c = nextChar ();
							
							if (c == '(')
							{
								skipToMatch ("()");
							}
						}
						
						c = skipLineComment();
						
						if (c == '(')
						{
							goto skipType;
						}
						
						if (isFunctionName (c)) {
							readSomething(c, name, (void *)isFunctionName);
							
							makeSimpleTag (name, AnubisKinds, K_FUNCTION);
						}
					}
				
					continue;
			
					inpar = TRUE;
				}
			}
			
			newline = FALSE;
		}
		
		if (c == '\n')
		{
			newline = TRUE;
			continue;
		}
    /*
    	if (inpar)
    	{
			if (isspace (c))
			{
				continue;
			}
			else if (isIdentifier (c))
			{
				readIdentifier (c, name);
			}
			else if (c == '\'')
			{
				skipToMatch ("\'\'");
			}
			else if (c == '"')
			{
				skipToMatch ("\"\"");
			}
			else if (c == '[')
			{
				skipToMatch ("[]");
			}
			else if (c == '(')
			{
				skipToMatch ("()");
			}
			else if (c == '.')
			{
				inpar = FALSE;
			}
    	}*/
    }
    vStringDelete (name);
}

/* Create parser definition stucture */
extern parserDefinition* AnubisParser (void)
{
    static const char *const extensions [] = { "anubis", NULL };
    parserDefinition* def = parserNew ("Anubis");
    def->kinds      = AnubisKinds;
    def->kindCount  = KIND_COUNT (AnubisKinds);
    def->extensions = extensions;
    def->parser     = findAnubisTags;
    return def;
}
