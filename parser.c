/*
 * Compiz fragment program parser
 *
 * Author : Guillaume Seguin
 * Email : guillaume@segu.in
 *
 * Copyright (c) 2007 Guillaume Seguin <guillaume@segu.in>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <compiz.h>
#include "parser.h"

/*
 * Helper function to get the basename of file from its path
 * e.g. basename ("/home/user/blah.c") == "blah.c"
 * special case : basename ("/home/user/") == "user"
 */
char *
base_name (char *str)
{
    char *current = str;
    int length;
    while (*current)
    {
	if (*current == '/')
	{
	    /* '/' found, check if it is the latest char of the string,
	     * if not update result string pointer */
	    current++;
	    if (!*current) break;
	    str = current;
	}
	else
	    current++;
    }
    length = strlen (str);
    /* Duplicate result string for trimming */
    current = strdup (str);
    if (!current)
	return NULL;
    /* Trim terminating '/' if needed */
    if (current[(length - 1)] == '/')
	current[(length - 1)] = 0;
    return current;
}

/*
 * Left trimming function
 */
static char *
ltrim (char *string)
{
    while (*string && (*string == ' ' || *string == '\t'))
	string++;
    return string;
}

/*
 * File reader function
 */
static char *
programReadSource (char *fname)
{
    FILE   *fp;
    char   *data, *path = NULL, *home = getenv ("HOME");
    int     length;

    /* Try to open file fname as is */
    fp = fopen (fname, "r");

    /* If failed, try as user filter file (in ~/.compiz/filters) */
    if (!fp && home && strlen (home))
    {
	asprintf (&path, "%s/.compiz/filters/%s", home, fname);
	fp = fopen (path, "r");
	free (path);
    }

    /* If failed again, try as system wide data file 
     * (in PREFIX/share/compiz/filters) */
    if (!fp)
    {
	asprintf (&path, "%s/share/compiz/filters/%s", PREFIX, fname);
	fp = fopen (path, "r");
	free (path);
    }

    /* If failed again & again, abort */
    if (!fp)
	return NULL;

    /* Get file length */
    fseek (fp, 0L, SEEK_END);
    length = ftell (fp);
    rewind (fp);

    /* Alloc memory */
    data = malloc (sizeof (char) * (length + 1));
    if (!data)
	return NULL;

    /* Read file */
    fread (data, length, 1, fp);

    data[length] = 0;

    /* Close file */
    fclose (fp);

    return data;
}

/*
 * Get the first "argument" in the given string, trimmed
 * and move source string pointer after the end of the argument.
 * For instance in string " foo, bar" this function will return "foo".
 *
 * This function returns NULL if no argument found
 * or a malloc'ed string that will have to be freed later.
 */
static char *
getFirstArgument (char **source)
{
    char *next, *arg;
    char *string, *orig;
    int length;

    if (!**source)
	return NULL;

    /* Left trim */
    orig = string = ltrim (*source);

    /* Find next comma or semicolon (which isn't that useful since we
     * are working on tokens delimited by semicolons) */
    if ((next = strstr (string, ",")) || (next = strstr (string, ";")))
    {
	length = next - string;
	if (!length)
	{
	    (*source)++;
	    return getFirstArgument (source);
	}
    }
    else
    {
	length = strlen (string);
    }

    /* Allocate, copy and end string */
    arg = malloc (sizeof (char) * (length + 1));
    if (!arg) return NULL;

    strncpy (arg, string, length);
    arg[length] = 0;

    /* Increment source pointer */
    if (string - orig + strlen (arg) + 1 <= strlen (*source))
	*source += string - orig + strlen (arg) + 1;
    else
	**source = 0;

    return arg;
}

/*
 * Parse the source buffer op by op and add each op to function data
 */
static void
programParseSource (CompFunctionData *data,
		    int target, char *source)
{
    char *line, *next, *current;
    char *strtok_ptr;
    int   length, oplength, type;
    Bool colorDone = FALSE;

    char *arg1, *arg2;

    /* Find the header, skip it, and start parsing from there */
    while (*source)
    {
	if (strncmp (source, "!!ARBfp1.0", 10) == 0)
	{
	    source += 10;
	    break;
	}
	source++;
    }

    /* Strip linefeeds */
    next = source;
    while ((next = strstr (next, "\n")))
	*next = ' ';

    line = strtok_r (source, ";", &strtok_ptr);
    /* Parse each instruction */
    while (line)
    {
	line = strdup (line);
	current = ltrim (line);

	/* Find instruction type */
	type = NoOp;

	/* Comments */
	if (strncmp (current, "#", 1) == 0)
	{
	    free (line);
	    line = strtok_r (NULL, ";", &strtok_ptr);
	    continue;
	}
	if ((next = strstr (current, "#")))
	    *next = 0;

	/* Data ops */
	if (strncmp (current, "END", 3) == 0)
	    type = NoOp;
	else if (strncmp (current, "ABS", 3) == 0
	      || strncmp (current, "ADD", 3) == 0
	      || strncmp (current, "CMP", 3) == 0
	      || strncmp (current, "COS", 3) == 0
	      || strncmp (current, "DP3", 3) == 0
	      || strncmp (current, "DP4", 3) == 0
	      || strncmp (current, "EX2", 3) == 0
	      || strncmp (current, "FLR", 3) == 0
	      || strncmp (current, "FRC", 3) == 0
	      || strncmp (current, "KIL", 3) == 0
	      || strncmp (current, "LG2", 3) == 0
	      || strncmp (current, "LIT", 3) == 0
	      || strncmp (current, "LRP", 3) == 0
	      || strncmp (current, "MAD", 3) == 0
	      || strncmp (current, "MAX", 3) == 0
	      || strncmp (current, "MIN", 3) == 0
	      || strncmp (current, "POW", 3) == 0
	      || strncmp (current, "RCP", 3) == 0
	      || strncmp (current, "RSQ", 3) == 0
	      || strncmp (current, "SCS", 3) == 0
	      || strncmp (current, "SIN", 3) == 0
	      || strncmp (current, "SGE", 3) == 0
	      || strncmp (current, "SLT", 3) == 0
	      || strncmp (current, "SUB", 3) == 0
	      || strncmp (current, "SWZ", 3) == 0
	      || strncmp (current, "TXB", 3) == 0
	      || strncmp (current, "TXP", 3) == 0
	      || strncmp (current, "XPD", 3) == 0)
		type = DataOp;
	else if (strncmp (current, "TEMP", 4) == 0)
	    type = TempOp;
	else if (strncmp (current, "PARAM", 5) == 0)
	    type = ParamOp;
	else if (strncmp (current, "ATTRIB", 6) == 0)
	    type = AttribOp;
	else if (strncmp (current, "TEX", 3) == 0)
	    type = FetchOp;
	else if (strncmp (current, "MUL", 3) == 0)
	{
	    if (strstr (current, "fragment.color"))
		type = ColorOp;
	    else
		type = DataOp;
	}
	else if (strncmp (current, "MOV", 3) == 0)
	{
	    if (strstr (current, "result.color"))
		type = ColorOp;
	    else
		type = DataOp;
	}
	switch (type)
	{
	    /* Data op : just copy paste the whole instruction plus a ";" */
	    case DataOp:
		length = strlen (current);
		arg1 = malloc (sizeof (char) * (length + 2));
		strncpy (arg1, current, length);
		arg1[length] = ';';
		arg1[length + 1] = 0;
		addDataOpToFunctionData (data, arg1);
		free (arg1);
		break;
	    /* Parse arguments one by one */
	    case TempOp:
	    case AttribOp:
	    case ParamOp:
		if (type == TempOp) oplength = 4;
		else if (type == ParamOp) oplength = 5;
		else if (type == AttribOp) oplength = 6;
		length = strlen (current);
		if (length < oplength + 2) break;
		current += oplength + 1;
		while (current && *current &&
		       (arg1 = getFirstArgument (&current)))
		{
		    /* "output" is a reserved word, skip it */
		    if (strncmp (arg1, "output", 6) == 0)
		    {
			free (arg1);
			continue;
		    }
		    /* Add ops */
		    if (type == TempOp)
			addTempHeaderOpToFunctionData (data, arg1);
		    else if (type == ParamOp)
			addParamHeaderOpToFunctionData (data, arg1);
		    else if (type == AttribOp)
			addAttribHeaderOpToFunctionData (data, arg1);
		    free (arg1);
		}
		break;
	    case FetchOp:
		current = strstr (current, " ") + 1;
		length = strstr (current, ",") - current;
		if (length < 1) break;
		arg1 = malloc (sizeof (char) * (length + 1));
		if (!arg1) break;
		strncpy (arg1, current, length);
		arg1[length] = 0;
		addFetchOpToFunctionData (data, arg1, NULL, target);
		break;
	    case ColorOp:
		if (colorDone) break;
		if (strncmp (current, "MUL", 3) == 0) /* MUL op, 2 ops */
		{
		    /* Example : MUL output, fragment.color, output;
		     * MOV arg1, fragment.color, arg2 */
		    current = strstr (current, " ") + 1;
		    length = strstr (current, ",") - current;
		    if (length < 1) break;
		    arg1 = malloc (sizeof (char) * (length + 1));
		    if (!arg1) break;
		    strncpy (arg1, current, length);
		    arg1[length] = 0;
		    if (strlen (current) < 3)
		    {
			free (arg1);
			break;
		    }
		    current = strstr (current, ",") + 2;
		    if (strlen (current) < 3)
		    {
			free (arg1);
			break;
		    }
		    current = strstr (current, ",") + 2;
		    length = strlen (current);
		    if (length < 1) break;
		    arg2 = malloc (sizeof (char) * (length + 1));
		    if (!arg2)
		    {
			free (arg1);
			break;
		    }
		    strncpy (arg2, current, length);
		    arg2[length] = 0;
		    addColorOpToFunctionData (data, arg1, arg2);
		    free (arg1);
		    free (arg2);
		}
		else /* MOV op, 1 op */
		{
		    /* Example : MOV result.color, output;
		     * MOV result.color, arg1; */
		    current = strstr (current, ",") + 1;
		    if ((arg1 = getFirstArgument (&current)))
		    {
			addColorOpToFunctionData (data, "output", arg1);
			free (arg1);
		    }
		}
		colorDone = TRUE;
		break;
	    default:
		break;
	}
	free (line);
	line = strtok_r (NULL, ";", &strtok_ptr);
    }
}

/*
 * Build a Compiz Fragment Function from a source string
 */
int
buildFragmentProgram (char *source, char *name,
		      CompScreen *s, int target)
{
    CompFunctionData *data;
    int handle;
    /* Create the function data */
    data = createFunctionData ();
    /* Parse the source and fill the function data */
    programParseSource (data, target, source);
    /* Create the function */
    handle = createFragmentFunction (s, name, data);
    /* Clean things */
    destroyFunctionData (data);
    return handle;
}

/*
 * Load a source file and build a Compiz Fragment Function from it
 */
int
loadFragmentProgram (char *file, char *name,
		     CompScreen *s, int target)
{
    char *source;
    int handle;
    /* Read the source file */
    source = programReadSource (file);
    if (!source)
	return 0;
    /* Build the Compiz Fragment Program */
    handle = buildFragmentProgram (source, name, s, target);
    free (source);
    return handle;
}
