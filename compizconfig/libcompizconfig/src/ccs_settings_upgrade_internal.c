/*
 * Compiz configuration system library
 *
 * Copyright (C) 2007  Dennis Kasprzyk <onestone@opencompositing.org>
 * Copyright (C) 2007  Danny Baumann <maniac@opencompositing.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "ccs_settings_upgrade_internal.h"
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

int
ccsUpgradeNameFilter (const char *name)
{
    int length = strlen (name);
    char *uname, *tok;

    if (length < 7)
	return 0;

    uname = tok = strdup (name);

    /* Keep removing domains and other bits
     * until we hit a number that we can parse */
    while (tok)
    {
	long int num = 0;
	char *nexttok = strchr (tok, '.') + 1;
	char *nextnexttok = strchr (nexttok, '.') + 1;
	char *end;
	char *bit = strndup (nexttok, strlen (nexttok) - (strlen (nextnexttok) + 1));

	/* FIXME: That means that the number can't be a zero */
	errno = 0;
	num = strtol (bit, &end, 10);

	if (!(errno != 0 && num == 0) &&
	    end != bit)
	{
	    free (bit);

	    /* Check if the next token after the number
	     * is .upgrade */

	    if (strncmp (nextnexttok, "upgrade", 7))
		return 0;
	    break;
	}
	else if (errno)
	    perror ("strtol");

	tok = nexttok;

	free (bit);
    }

    free (uname);

    return 1;
}

Bool
ccsUpgradeGetDomainNumAndProfile (const char   *name,
				  char         **domain,
				  unsigned int *num,
				  char         **profile)
{
    int length = strlen (name);
    const char *tok = name;
    Bool       success = FALSE;

    /* Keep removing domains and other bits
     * until we hit a number that we can parse */
    while (tok)
    {
	long int numTmp = 0;
	char *nexttok = strchr (tok, '.') + 1;
	char *nextnexttok = strchr (nexttok, '.') + 1;
	char *end;
	char *bit = strndup (nexttok, strlen (nexttok) - (strlen (nextnexttok) + 1));

	/* FIXME: That means that the number can't be a zero */
	errno = 0;
	numTmp = strtol (bit, &end, 10);

	if (!(errno != 0 && numTmp == 0) &&
	    end != bit)
	{
	    *domain = strndup (name, length - (strlen (tok) + 1));
	    *num = numTmp;

	    /* profile.n.upgrade */
	    *profile = strndup (tok, strlen (tok) - (strlen (nexttok) + 1));

	    success = TRUE;
	}
	else if (errno)
	    perror ("strtol");

	tok = nexttok;
	free (bit);

	if (success)
	    return TRUE;
    }

    return FALSE;
}
