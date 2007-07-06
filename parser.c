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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <compiz.h>
#include "parser.h"

/*
 * Helper function to get the basename of file from its path
 * e.g. basename ("/home/user/blah.c") == "blah.c"
 * special case : basename ("/home/user/") == "user/"
 */
char *basename (char *str)
{
    char *current = str;
    while (1)
    {
        if (*current == '\0') break;
        if (*current == '/')
        {
            current++;
            if (*current == '\0') break;
            str = current;
        }
        else
            current++;
    }
    return str;
}

/*
 * File reader function
 */
static char *programReadSource (char *fname)
{
    FILE   *fp;
    char   *data;
    int     length;

    // Open file
    fp = fopen (fname, "r");

    // Get file length
    fseek (fp, 0L, SEEK_END);
    length = ftell (fp);
    rewind (fp);

    // Alloc memory
    data = malloc (sizeof (char) * (length + 1));
    if (!data)
        return NULL;

    // Read file
    fread (data, length, 1, fp);

    data[length] = '\0';

    // Close file
    fclose (fp);

    return data;
}

/*
 * Parse the source buffer op by op and add each op to function data
 */
static void programParseSource (CompFunctionData *data,
                                int target, char *source)
{
    char *line, *next, *current;
    int   length, oplength, type;
    Bool colorDone = FALSE;

    char *arg1, *arg2;

    // Find the header, skip it, and start parsing from there
    while (*source != '\0')
    {
        if (strncmp (source, "!!ARBfp1.0", 10) == 0)
        {
            source += 10;
            break;
        }
        source++;
    }

    // Strip linefeeds
    next = source;
    while ((next = strstr (next, "\n")))
        *next = ' ';

    line = strtok (source, ";");
    // Parse each instruction
    while (line)
    {
        current = line;
        // Left trim it
        while (*current != '\0' && (*current == ' ' || *current == '\t'))
            current++;
        // Find instruction type
        type = NoOp;
        if (strncmp (current, "ABS", 3) == 0
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
            if (strstr (current, "fragment.color") != current)
                type = ColorOp;
            else
                type = DataOp;
        }
        else if (strncmp (current, "MOV", 3) == 0)
        {
            if (strstr (current, "result.color") != current)
                type = ColorOp;
            else
                type = DataOp;
        }
        //else if (strncmp (current, "", 1) == 0)
        //    type = DataOp;
        switch (type)
        {
            // Data op : just paste the whole instruction
            case DataOp:
                length = strlen (current);
                arg1 = malloc (sizeof (char) * (length + 2));
                strncpy (arg1, current, length);
                arg1[length] = ';';
                arg1[length + 1] = '\0';
                addDataOpToFunctionData (data, arg1);
                free (arg1);
                break;
            // This is a 1-arg op, just parse the arg
            case TempOp:
            case AttribOp:
            case ParamOp:
                if (type == TempOp) oplength = 4;
                else if (type == ParamOp) oplength = 5;
                else if (type == AttribOp) oplength = 6;
                length = strlen (current);
                if (length < oplength + 2) break;
                arg1 = malloc (sizeof (char) * (length - oplength - 1));
                if (!arg1) break;
                current += oplength + 1;
                strncpy (arg1, current, length - oplength - 1);
                arg1[length - oplength - 1] = '\0';
                if (strncmp (arg1, "output", 6) == 0)
                {
                    free (arg1);
                    break;
                }
                // Add ops
                if (type == TempOp)
                    addTempHeaderOpToFunctionData (data, arg1);
                else if (type == ParamOp)
                    addParamHeaderOpToFunctionData (data, arg1);
                else if (type == AttribOp)
                    addAttribHeaderOpToFunctionData (data, arg1);
                free (arg1);
                break;
            case FetchOp:
                current = strstr (current, " ") + 1;
                length = strstr (current, ",") - current;
                if (length < 1) break;
                arg1 = malloc (sizeof (char) * (length + 1));
                if (!arg1) break;
                strncpy (arg1, current, length);
                arg1[length] = '\0';
                addFetchOpToFunctionData (data, arg1, NULL, target);
                break;
            case ColorOp:
                if (colorDone) break;
                if (strncmp (current, "MUL", 3) == 0) // MUL op, 2 ops
                {
                    current = strstr (current, " ") + 1;
                    length = strstr (current, ",") - current;
                    if (length < 1) break;
                    arg1 = malloc (sizeof (char) * (length + 1));
                    if (!arg1) break;
                    strncpy (arg1, current, length);
                    arg1[length] = '\0';
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
                    arg2[length] = '\0';
                    addColorOpToFunctionData (data, arg1, arg2);
                    free (arg1);
                    free (arg2);
                }
                else // MOV op, 1 op
                {
                    current = strstr (current, ",") + 2;
                    length = strlen (current);
                    if (length < 1) break;
                    arg1 = malloc (sizeof (char) * (length + 1));
                    if (!arg1) break;
                    strncpy (arg1, current, length);
                    arg1[length] = '\0';
                    addColorOpToFunctionData (data, arg1, arg1);
                    free (arg1);
                }
                colorDone = TRUE;
                break;
            default:
                break;
        }
        source = next + 1;
        line = strtok (NULL, ";");
    }
}

// Pack everything together into this final 
int loadFragmentProgram (char *file, char *name,
                         CompScreen *s, int target)
{
    char *source;
    int handle = 0;
    CompFunctionData *data;
    source = programReadSource (file);
    if (!source)
        return 0;
    data = createFunctionData ();
    programParseSource (data, target, source);
    handle = createFragmentFunction (s, name, data);
    destroyFunctionData (data);
    free (source);
    return handle;
}
