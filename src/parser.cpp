/*
 * Compiz fragment program parser
 *
 * parser.cpp
 *
 * This should be usable on almost any plugin that wishes to parse fragment
 * program files separately, maybe it should become a separate plugin?
 *
 * Author : Guillaume Seguin
 * Email : guillaume@segu.in
 *
 * Copyright (c) 2007 Guillaume Seguin <guillaume@segu.in>
 *
 * Basic C++ port of this by:
 * Copyright (c) 2009 Sam Spilsbury <smspillaz@gmail.com>
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

/* FIXME: This is a really really really really really really really really
	  really really really really really really terrible C++ port. It could
	  have been done SO much better but I honestly suck at working with
	  strings. Can someone fix this to remove all the uses of char * and
	  replace with CompString &? Thanks.
		-Sam Spilsbury <smspillaz@gmail.com>
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <cstring>
#include <ctype.h>

#include <iostream>
#include <fstream>
#include "parser.h"

/* General helper functions ------------------------------------------------- */

/*
 * Helper function to get the basename of file from its path
 * e.g. basename ("/home/user/blah.c") == "blah.c"
 * special case : basename ("/home/user/") == "user"
 */
CompString
FragmentParser::base_name (CompString str)
{
    size_t pos = 0, foundPos = 0;
    unsigned int length;
    while (foundPos != std::string::npos)
    {
	foundPos = str.find ("/", pos);
	if (foundPos != std::string::npos)
	{
	    /* '/' found, check if it is the latest char of the string,
	     * if not update result string pointer */
	    if (pos + 1 > str.size ())
		break;

	    pos = foundPos;
	}
    }
    length = str.size ();
    /* Trim terminating '/' if needed */
    if (length > 0 && str.at (length - 1) == '/')
	str = str.substr (pos, length - 1);
    return str;
}

/*
 * Left trimming function
 */
CompString
FragmentParser::ltrim (CompString string)
{
    size_t pos = 0;
    while (!(pos > string.size ()))
    {
	if (string.at (pos) == ' ' || string.at (pos) == '\t')
	    pos++;
	else
	    break;
    }

    return string.substr (pos);
}

/* General fragment program related functions ------------------------------- */

/*
 * Clean program name string
 */
CompString
FragmentParser::programCleanName (CompString name)
{
    unsigned int pos = 0;
    CompString bit ("_foo");

    /* Strange hack (gcc seems not to like "_", but will take
     * things like "_foo" for whatever reason) */
    bit = bit.substr (0, 1);

    /* Replace every non alphanumeric char by '_' */
    while (!(pos >= name.size ()))
    {
	if (!isalnum (name.at (pos)))
	    name.replace (pos, 1, bit);

	pos++;
    }

    return name;
}

/*
 * File reader function
 */
CompString
FragmentParser::programReadSource (CompString fname)
{
    std::ifstream fp;
    CompString data, path, home = CompString (getenv ("HOME"));
    CompString retData;

    /* Try to open file fname as is */
    fp.open ("filename.ext");

    /* If failed, try as user filter file (in ~/.compiz/data/filters) */
    if (!fp.is_open () && !home.empty ())
    {
	path = home + "/.compiz/data/filters/" + fname;
	fp.open (path.c_str ());
    }

    /* If failed again, try as system wide data file
     * (in PREFIX/share/compiz/filters) */
    if (!fp.is_open ())
    {
	path = CompString (DATADIR) + "/data/filters/" + fname;
	fp.open (path.c_str ());
    }

    /* If failed again & again, abort */
    if (!fp.is_open ())
    {
	return CompString ("");
    }

    /* Read file */
    while (fp.good ())
    {
	CompString line;

	std::getline (fp, line);
	retData += line;
    }

    fp.close ();

    return retData;
}

/*
 * Get the first "argument" in the given string, trimmed
 * and move source string pointer after the end of the argument.
 * For instance in string " foo, bar" this function will return "foo".
 *
 * This function returns NULL if no argument found
 * or a malloc'ed string that will have to be freed later.
 */
CompString
FragmentParser::getFirstArgument (char **source)
{
    char *next, *arg, *temp;
    char *string, *orig;
    int length;
    CompString retArg;

    if (!**source)
	return NULL;

    /* Left trim */
    orig = string = strdup (ltrim (CompString (*source)).c_str ());

    /* Find next comma or semicolon (which isn't that useful since we
     * are working on tokens delimited by semicolons) */
    if ((next = strstr (string, ",")) || (next = strstr (string, ";")))
    {
	length = next - string;
	if (!length)
	{
	    (*source)++;
	    free (orig);
	    return getFirstArgument (source);
	}
	if ((temp = strstr (string, "{")) && temp < next &&
	    (temp = strstr (string, "}")) && temp > next)
	{
	    if ((next = strstr (temp, ",")) || (next = strstr (temp, ";")))
		length = next - string;
	    else
		length = strlen (string);
	}
    }
    else
    {
	length = strlen (string);
    }

    /* Allocate, copy and end string */
    arg = (char *) malloc (sizeof (char) * (length + 1));
    if (!arg)
    {
        free (orig);
	return NULL;
    }

    strncpy (arg, string, length);
    arg[length] = 0;

    /* Increment source pointer */
    if (string - orig + strlen (arg) + 1 <= strlen (*source))
	*source += string - orig + strlen (arg) + 1;
    else
	**source = 0;

    retArg = CompString (arg);

    free (arg);
    free (orig);

    return retArg;
}

/* Texture offset related functions ----------------------------------------- */

/*
 * Add a new fragment offset to the offsets stack from an ADD op string
 */
FragmentParser::FragmentOffset *
FragmentParser::programAddOffsetFromAddOp (char *source)
{
    FragmentOffset  *offset;
    char	    *op, *orig_op;
    char	    *name;
    char	    *offset_string;
    char	    *temp;
    std::list <FragmentOffset *>::iterator it = offsets.begin ();

    if (strlen (source) < 5)
	return offsets.front ();

    orig_op = op = strdup (source);

    op += 3;
    if (!(name = strdup (getFirstArgument (&op).c_str ())))
    {
	free (orig_op);
	return offsets.front ();
    }

    /* If an offset with the same name is already registered, skeep this one */
    if (!programFindOffset (it, name).empty () ||
	!(temp = strdup (getFirstArgument (&op).c_str ())))
    {
	free (name);
	free (orig_op);
	return (*it); // ???
    }

    /* We don't need this, let's free it immediately */
    free (temp);

    /* Just use the end of the op as the offset */
    op += 1;
    offset_string = strdup (ltrim (op).c_str ());
    if (!offset_string)
    {
	free (name);
	free (orig_op);
	return offsets.front ();
    }

    offset = new FragmentOffset ();
    if (!offset)
    {
	free (offset_string);
	free (name);
	free (orig_op);
	return (*it);
    }

    offset->name =  CompString (name);
    offset->offset = CompString (offset_string);

    offsets.push_back (offset);

    free (offset_string);
    free (name);
    free (orig_op);

    return offset;
}

/*
 * Find an offset according to its name
 */
CompString
FragmentParser::programFindOffset (std::list<FragmentOffset *>::iterator it,
				   char *name)
{
    if (!(*it))
	return NULL;

    if ((*it)->name.compare (CompString (name)) == 0)
	return CompString ((*it)->offset);

    return programFindOffset ((it++), name);
}

/*
 * Recursively free offsets stack
 */
void
FragmentParser::programFreeOffset ()
{
    offsets.clear ();
}

/* Actual parsing/loading functions ----------------------------------------- */

/*
 * Parse the source buffer op by op and add each op to function data
 */
/* FIXME : I am more than 200 lines long, I feel so heavy! */
void
FragmentParser::programParseSource (GLFragment::FunctionData *data,
		    		    int target, char *source)
{
    char *line, *next, *current;
    char *strtok_ptr;
    int   length, oplength, type;
    FragmentOffset *offset = NULL;

    char *arg1, *arg2, *temp;

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
	char *origcurrent = current = strdup (ltrim (line).c_str ());

	/* Find instruction type */
	type = NoOp;

	/* Comments */
	if (strncmp (current, "#", 1) == 0)
	{
	    free (line);
	    free (current);
	    line = strtok_r (NULL, ";", &strtok_ptr);
	    continue;
	}
	if ((next = strstr (current, "#")))
	    *next = 0;

	/* Data ops */
	if (strncmp (current, "END", 3) == 0)
	    type = NoOp;
	else if (!strncmp (current, "ABS", 3) || !strncmp (current, "CMP", 3) ||
	         !strncmp (current, "COS", 3) || !strncmp (current, "DP3", 3) ||
	         !strncmp (current, "DP4", 3) || !strncmp (current, "EX2", 3) ||
	         !strncmp (current, "FLR", 3) || !strncmp (current, "FRC", 3) ||
	         !strncmp (current, "KIL", 3) || !strncmp (current, "LG2", 3) ||
	         !strncmp (current, "LIT", 3) || !strncmp (current, "LRP", 3) ||
	         !strncmp (current, "MAD", 3) || !strncmp (current, "MAX", 3) ||
	         !strncmp (current, "MIN", 3) || !strncmp (current, "POW", 3) ||
	         !strncmp (current, "RCP", 3) || !strncmp (current, "RSQ", 3) ||
	         !strncmp (current, "SCS", 3) || !strncmp (current, "SIN", 3) ||
	         !strncmp (current, "SGE", 3) || !strncmp (current, "SLT", 3) ||
	         !strncmp (current, "SUB", 3) || !strncmp (current, "SWZ", 3) ||
	         !strncmp (current, "TXB", 3) || !strncmp (current, "TXP", 3) ||
	         !strncmp (current, "XPD", 3))
		type = DataOp;
	else if (strncmp (current, "TEMP", 4) == 0)
	    type = TempOp;
	else if (strncmp (current, "PARAM", 5) == 0)
	    type = ParamOp;
	else if (strncmp (current, "ATTRIB", 6) == 0)
	    type = AttribOp;
	else if (strncmp (current, "TEX", 3) == 0)
	    type = FetchOp;
	else if (strncmp (current, "ADD", 3) == 0)
	{
	    if (strstr (current, "fragment.texcoord"))
		offset = programAddOffsetFromAddOp (current); // ???
	    else
		type = DataOp;
	}
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
		asprintf (&arg1, "%s;", current);
		data->addDataOp (arg1);
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
		       (arg1 = strdup (getFirstArgument (&current).c_str ())))
		{
		    /* "output" is a reserved word, skip it */
		    if (strncmp (arg1, "output", 6) == 0)
		    {
			free (arg1);
			continue;
		    }
		    /* Add ops */
		    if (type == TempOp)
			data->addTempHeaderOp (arg1);
		    else if (type == ParamOp)
			data->addParamHeaderOp (arg1);
		    else if (type == AttribOp)
			data->addAttribHeaderOp (arg1);
		    free (arg1);
		}
		break;
	    case FetchOp:
		/* Example : TEX tmp, coord, texture[0], RECT;
		 * "tmp" is dest name, while "coord" is either
		 * fragment.texcoord[0] or an offset */
		current += 3;
		if ((arg1 = strdup (getFirstArgument (&current).c_str ())))
		{
		    if (!(temp = strdup (getFirstArgument (&current).c_str ())))
		    {
			free (arg1);
			break;
		    }
		    if (strcmp (temp, "fragment.texcoord[0]") == 0)
			data->addFetchOp (arg1, NULL, target);
		    else
		    {
			arg2 = strdup (programFindOffset (
					      offsets.begin (), temp).c_str ());
			if (arg2)
			{
			    data->addFetchOp (arg1, arg2, target);
			    free (arg2);
			}
		    }
		    free (arg1);
		    free (temp);
		}
		break;
	    case ColorOp:
		if (strncmp (current, "MUL", 3) == 0) /* MUL op, 2 ops */
		{
		    /* Example : MUL output, fragment.color, output;
		     * MOV arg1, fragment.color, arg2 */
		    current += 3;
		    if  (!(arg1 = strdup (getFirstArgument (
						&current).c_str ())))
			break;

		    if (!(temp = strdup (getFirstArgument (&current).c_str ())))
		    {
			free (arg1);
			break;
		    }

		    free (temp);

		    if (!(arg2 = strdup (getFirstArgument (&current).c_str ())))
		    {
			free (arg1);
			break;
		    }

		    data->addColorOp (arg1, arg2);
		    free (arg1);
		    free (arg2);
		}
		else /* MOV op, 1 op */
		{
		    /* Example : MOV result.color, output;
		     * MOV result.color, arg1; */
		    current = strstr (current, ",") + 1;
		    if ((arg1 = strdup (getFirstArgument (&current).c_str ())))
		    {
			data->addColorOp ("output", arg1);
			free (arg1);
		    }
		}
		break;
	    default:
		break;
	}
	free (origcurrent);
	free (line);
	line = strtok_r (NULL, ";", &strtok_ptr);
    }
    programFreeOffset ();
    offset = NULL;
}

/*
 * Build a Compiz Fragment Function from a source string
 */
GLFragment::FunctionId
FragmentParser::buildFragmentProgram (char *source, char *name, int target)
{
    GLFragment::FunctionData *data;
    int handle;
    /* Create the function data */
    data = new GLFragment::FunctionData ();
    if (!data)
	return 0;
    /* Parse the source and fill the function data */
    programParseSource (data, target, source);
    /* Create the function */
    handle = data->createFragmentFunction (name);
    /* Clean things */
    delete data;
    return handle;
}

/*
 * Load a source file and build a Compiz Fragment Function from it
 */
GLFragment::FunctionId
FragmentParser::loadFragmentProgram (CompString &file,
				     CompString &name,
				     int target)
{
    CompString source;
    GLFragment::FunctionId handle;
    char *source_c, *name_c;

    /* Clean fragment program name */
    name = programCleanName (name);
    /* Read the source file */
    source = programReadSource (file);
    if (source.empty ())
    {
	return 0;
    }
    source_c = strdup (source.c_str ());
    name_c   = strdup (name.c_str ());

    /* Build the Compiz Fragment Program */
    handle = buildFragmentProgram (source_c,
				   name_c, target);

    free (name_c);
    free (source_c);
    return handle;
}
