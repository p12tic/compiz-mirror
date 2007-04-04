/*
 * Copyright © 2007 David Reveman
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * David Reveman not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * David Reveman makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * DAVID REVEMAN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL DAVID REVEMAN BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: David Reveman <davidr@novell.com>
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <sys/mount.h>
#include <fuse.h>
#include <fuse_lowlevel.h>

#include <compiz.h>

#define FUSE_MOUNT_POINT_DEFAULT "compiz"

#define FUSE_INODE_TYPE_ROOT        (1 << 0)
#define FUSE_INODE_TYPE_CORE        (1 << 1)
#define FUSE_INODE_TYPE_PLUGIN      (1 << 2)
#define FUSE_INODE_TYPE_SCREEN      (1 << 3)
#define FUSE_INODE_TYPE_DISPLAY     (1 << 4)
#define FUSE_INODE_TYPE_OPTION      (1 << 5)
#define FUSE_INODE_TYPE_SHORT_DESC  (1 << 6)
#define FUSE_INODE_TYPE_LONG_DESC   (1 << 7)
#define FUSE_INODE_TYPE_TYPE        (1 << 8)
#define FUSE_INODE_TYPE_VALUE       (1 << 9)
#define FUSE_INODE_TYPE_ITEM_COUNT  (1 << 10)
#define FUSE_INODE_TYPE_ITEM_TYPE   (1 << 11)
#define FUSE_INODE_TYPE_ITEMS       (1 << 12)
#define FUSE_INODE_TYPE_ITEM_VALUE  (1 << 13)
#define FUSE_INODE_TYPE_KEY         (1 << 14)
#define FUSE_INODE_TYPE_BUTTON      (1 << 15)
#define FUSE_INODE_TYPE_EDGE        (1 << 16)
#define FUSE_INODE_TYPE_EDGE_BUTTON (1 << 17)
#define FUSE_INODE_TYPE_BELL        (1 << 18)
#define FUSE_INODE_TYPE_MIN         (1 << 19)
#define FUSE_INODE_TYPE_MAX         (1 << 20)
#define FUSE_INODE_TYPE_PRECISION   (1 << 21)
#define FUSE_INODE_TYPE_SELECTION   (1 << 22)
#define FUSE_INODE_TYPE_ALTERNATIVE (1 << 23)

#define DIR_MASK (FUSE_INODE_TYPE_ROOT    | \
		  FUSE_INODE_TYPE_CORE    | \
		  FUSE_INODE_TYPE_PLUGIN  | \
		  FUSE_INODE_TYPE_SCREEN  | \
		  FUSE_INODE_TYPE_DISPLAY | \
		  FUSE_INODE_TYPE_OPTION  | \
		  FUSE_INODE_TYPE_ITEMS   | \
		  FUSE_INODE_TYPE_SELECTION)

#define CONST_DIR_MASK (FUSE_INODE_TYPE_CORE    | \
			FUSE_INODE_TYPE_PLUGIN  | \
			FUSE_INODE_TYPE_SCREEN  | \
			FUSE_INODE_TYPE_DISPLAY | \
			FUSE_INODE_TYPE_OPTION  | \
			FUSE_INODE_TYPE_SELECTION)

#define ACTION_MASK (FUSE_INODE_TYPE_KEY	 | \
		     FUSE_INODE_TYPE_BUTTON	 | \
		     FUSE_INODE_TYPE_EDGE	 | \
		     FUSE_INODE_TYPE_EDGE_BUTTON | \
		     FUSE_INODE_TYPE_BELL)

#define WRITE_MASK (FUSE_INODE_TYPE_VALUE	| \
		    FUSE_INODE_TYPE_ITEM_VALUE	| \
		    FUSE_INODE_TYPE_KEY		| \
		    FUSE_INODE_TYPE_BUTTON	| \
		    FUSE_INODE_TYPE_EDGE	| \
		    FUSE_INODE_TYPE_EDGE_BUTTON | \
		    FUSE_INODE_TYPE_BELL)

#define FUSE_INODE_FLAG_TRUNC (1 << 0)

typedef struct _FuseInode {
    struct _FuseInode *parent;
    struct _FuseInode *child;
    struct _FuseInode *sibling;

    int	       type;
    int	       flags;
    fuse_ino_t ino;
    char       *name;
} FuseInode;

typedef struct _FuseWriteBuffer {
    char *data;
    int  size;
    Bool dirty;
} FuseWriteBuffer;

static int displayPrivateIndex;

#define FUSE_DISPLAY_OPTION_MOUNT_POINT 0
#define FUSE_DISPLAY_OPTION_NUM		1

typedef struct _FuseDisplay {
    CompOption opt[FUSE_DISPLAY_OPTION_NUM];

    struct fuse_session *session;
    struct fuse_chan    *channel;
    char		*mountPoint;
    CompWatchFdHandle   watchFdHandle;
    char		*buffer;
} FuseDisplay;

#define GET_FUSE_DISPLAY(d)				     \
    ((FuseDisplay *) (d)->privates[displayPrivateIndex].ptr)

#define FUSE_DISPLAY(d)			   \
    FuseDisplay *fd = GET_FUSE_DISPLAY (d)

#define NUM_OPTIONS(d) (sizeof ((d)->opt) / sizeof (CompOption))

static const char *edgeName[] = {
    "edge_left",
    "edge_right",
    "edge_top",
    "edge_bottom",
    "edge_top_left",
    "edge_top_right",
    "edge_bottom_left",
    "edge_bottom_right"
};

static fuse_ino_t nextIno = 1;
static FuseInode  *inodes = NULL;

static FuseInode *
fuseAddInode (FuseInode	 *parent,
	      int	 type,
	      const char *name)
{
    FuseInode *inode;

    inode = malloc (sizeof (FuseInode));
    if (!inode)
	return NULL;

    inode->parent  = parent;
    inode->sibling = NULL;
    inode->child   = NULL;
    inode->type    = type;
    inode->flags   = 0;
    inode->ino     = nextIno++;
    inode->name	   = strdup (name);

    if (parent)
    {
	if (parent->child)
	    inode->sibling = parent->child;

	parent->child = inode;
    }

    return inode;
}

static void
fuseRemoveInode (FuseInode *parent,
		 FuseInode *inode)
{
    while (inode->child)
	fuseRemoveInode (inode, inode->child);

    if (parent)
    {
	FuseInode *s, *prev = NULL;

	for (s = parent->child; s; s = s->sibling)
	{
	    if (s == inode)
		break;

	    prev = s;
	}

	if (prev)
	    prev->sibling = inode->sibling;
	else
	    parent->child = NULL;
    }

    if (inode)
    {
	if (inode->name)
	    free (inode->name);

	free (inode);
    }
}

static FuseInode *
fuseFindInode (FuseInode  *inode,
	       fuse_ino_t ino,
	       int	  mask)
{
    if (inode->ino != ino)
    {
	FuseInode *c = inode->child;

	inode = NULL;
	while (c)
	{
	    inode = fuseFindInode (c, ino, ~0);
	    if (inode)
		break;

	    c = c->sibling;
	}
    }

    if (inode && (inode->type & mask))
	return inode;

    return NULL;
}

static FuseInode *
fuseLookupChild (FuseInode  *inode,
		 const char *name)
{
    FuseInode *c;

    for (c = inode->child; c; c = c->sibling)
	if (strcmp (c->name, name) == 0)
	    return c;

    return NULL;
}

static CompOption *
fuseGetDisplayOptionsFromInode (CompDisplay *d,
				FuseInode   *inode,
				int	    *nOption)
{
    CompOption *option = NULL;

    if (inode->type & FUSE_INODE_TYPE_CORE)
    {
	option = compGetDisplayOptions (d, nOption);
    }
    else if (inode->type & FUSE_INODE_TYPE_PLUGIN)
    {
	CompPlugin *p;

	p = findActivePlugin (inode->name);
	if (p && p->vTable->getDisplayOptions)
	    option = (*p->vTable->getDisplayOptions) (p, d, nOption);
    }

    return option;
}

static CompOption *
fuseGetScreenOptionsFromInode (CompScreen *s,
			       FuseInode  *inode,
			       int	  *nOption)
{
    CompOption *option = NULL;

    if (inode->type & FUSE_INODE_TYPE_CORE)
    {
	option = compGetScreenOptions (s, nOption);
    }
    else if (inode->type & FUSE_INODE_TYPE_PLUGIN)
    {
	CompPlugin *p;

	p = findActivePlugin (inode->name);
	if (p && p->vTable->getScreenOptions)
	    option = (*p->vTable->getScreenOptions) (p, s, nOption);
    }

    return option;
}

static CompOption *
fuseGetOptionsFromInode (CompDisplay *d,
			 FuseInode   *inode,
			 int	     *nOption)
{
    CompOption *option = NULL;

    if (inode->type & FUSE_INODE_TYPE_SCREEN)
    {
	CompScreen *s;
	int	   screenNum = -1;

	sscanf (inode->name, "screen%d", &screenNum);

	for (s = d->screens; s; s = s->next)
	    if (s->screenNum == screenNum)
		break;

	if (s)
	    option = fuseGetScreenOptionsFromInode (s, inode->parent, nOption);
    }
    else if (inode->type & FUSE_INODE_TYPE_DISPLAY)
    {
	option = fuseGetDisplayOptionsFromInode (d, inode->parent, nOption);
    }

    return option;
}

static CompOption *
fuseGetOptionFromInode (CompDisplay *d,
			FuseInode   *inode)
{
    if (inode->type & (FUSE_INODE_TYPE_OPTION |
		       FUSE_INODE_TYPE_ITEMS  |
		       FUSE_INODE_TYPE_SELECTION))
    {
	CompOption *option;
	int	   nOption;

	if (inode->type & (FUSE_INODE_TYPE_ITEMS | FUSE_INODE_TYPE_SELECTION))
	    inode = inode->parent;

	option = fuseGetOptionsFromInode (d, inode->parent, &nOption);
	if (option)
	{
	    while (nOption--)
	    {
		if (strcmp (inode->name, option->name) == 0)
		    return option;

		option++;
	    }
	}
    }

    return NULL;
}

static char *
fuseGetStringFromInode (CompDisplay *d,
			FuseInode   *inode)
{
    CompOption *option;
    char       str[256];

    if (!inode->parent)
	return NULL;

    option = fuseGetOptionFromInode (d, inode->parent);
    if (!option)
	return NULL;

    if (inode->flags & FUSE_INODE_FLAG_TRUNC)
	return strdup ("");

    if (inode->type & FUSE_INODE_TYPE_TYPE)
    {
	return strdup (optionTypeToString (option->type));
    }
    else if (inode->type & FUSE_INODE_TYPE_SHORT_DESC)
    {
	return strdup (option->shortDesc);
    }
    else if (inode->type & FUSE_INODE_TYPE_LONG_DESC)
    {
	return strdup (option->longDesc);
    }
    else if (inode->type & (FUSE_INODE_TYPE_VALUE | FUSE_INODE_TYPE_ITEM_VALUE))
    {
	CompOptionValue *value = NULL;
	CompOptionType  type;

	if (inode->type & FUSE_INODE_TYPE_ITEM_VALUE)
	{
	    int i;

	    if (sscanf (inode->name, "value%d", &i))
	    {
		if (i < option->value.list.nValue)
		{
		    value = &option->value.list.value[i];
		    type  = option->value.list.type;
		}
	    }
	}
	else
	{
	    value = &option->value;
	    type  = option->type;
	}

	if (value)
	{
	    switch (type) {
	    case CompOptionTypeBool:
		return strdup (value->b ? "true" : "false");
	    case CompOptionTypeInt:
		snprintf (str, 256, "%d", value->i);
		return strdup (str);
	    case CompOptionTypeFloat:
		snprintf (str, 256, "%f", value->f);
		return strdup (str);
	    case CompOptionTypeString:
		return strdup (value->s);
	    case CompOptionTypeColor:
		return colorToString (value->c);
	    case CompOptionTypeMatch:
		return matchToString (&value->match);
	    default:
		break;
	    }
	}
    }
    else if (inode->type & FUSE_INODE_TYPE_MIN)
    {
	if (option->type == CompOptionTypeInt)
	    snprintf (str, 256, "%d", option->rest.i.min);
	else
	    snprintf (str, 256, "%f", option->rest.f.min);

	return strdup (str);
    }
    else if (inode->type & FUSE_INODE_TYPE_MAX)
    {
	if (option->type == CompOptionTypeInt)
	    snprintf (str, 256, "%d", option->rest.i.max);
	else
	    snprintf (str, 256, "%f", option->rest.f.max);

	return strdup (str);
    }
    else if (inode->type & FUSE_INODE_TYPE_PRECISION)
    {
	snprintf (str, 256, "%f", option->rest.f.precision);
	return strdup (str);
    }
    else if (inode->type & FUSE_INODE_TYPE_ITEM_COUNT)
    {
	snprintf (str, 256, "%d", option->value.list.nValue);
	return strdup (str);
    }
    else if (inode->type & FUSE_INODE_TYPE_ITEM_TYPE)
    {
	return strdup (optionTypeToString (option->value.list.type));
    }
    else if (inode->type & FUSE_INODE_TYPE_KEY)
    {
	if (option->value.action.type & CompBindingTypeKey)
	    return keyBindingToString (d, &option->value.action.key);
	else
	    return strdup ("Disabled");
    }
    else if (inode->type & FUSE_INODE_TYPE_BUTTON)
    {
	if (option->value.action.type & CompBindingTypeButton)
	    return buttonBindingToString (d, &option->value.action.button);
	else
	    return strdup ("Disabled");
    }
    else if (inode->type & FUSE_INODE_TYPE_EDGE)
    {
	int i;

	for (i = 0; i < sizeof (edgeName) / sizeof (edgeName[0]); i++)
	{
	    if (strcmp (inode->name, edgeName[i]) == 0)
	    {
		if (option->value.action.edgeMask & (1 << i))
		    return strdup ("true");
		else
		    return strdup ("false");
	    }
	}
    }
    else if (inode->type & FUSE_INODE_TYPE_EDGE_BUTTON)
    {
	if (option->value.action.type & CompBindingTypeEdgeButton)
	{
	    snprintf (str, 256, "Button%d", option->value.action.edgeButton);
	    return strdup (str);
	}
	else
	{
	    return strdup ("Disabled");
	}
    }
    else if (inode->type & FUSE_INODE_TYPE_BELL)
    {
	return strdup (option->value.action.bell ? "true" : "false");
    }
    else if (inode->type & FUSE_INODE_TYPE_ALTERNATIVE)
    {
	int i;

	if (sscanf (inode->name, "alternative%d", &i))
	{
	    if (i < option->rest.s.nString)
		return strdup (option->rest.s.string[i]);
	}
    }

    return NULL;
}

static void
fuseUpdateInode (CompDisplay *d,
		 FuseInode   *inode)
{
    CompScreen *s;
    CompPlugin *p;
    CompOption *option;
    char       str[256];

    if (inode->type & FUSE_INODE_TYPE_ROOT)
    {
	FuseInode *c;

	if (!fuseLookupChild (inode, "core"))
	    fuseAddInode (inode, FUSE_INODE_TYPE_CORE, "core");

	for (c = inode->child; c; c = c->sibling)
	{
	    if (c->type & FUSE_INODE_TYPE_CORE)
		continue;

	    if (!findActivePlugin (c->name))
		fuseRemoveInode (inode, c);
	}

	for (p = getPlugins (); p; p = p->next)
	    if (!fuseLookupChild (inode, p->vTable->name))
		fuseAddInode (inode, FUSE_INODE_TYPE_PLUGIN,
			      p->vTable->name);
    }
    else if (inode->type & (FUSE_INODE_TYPE_CORE | FUSE_INODE_TYPE_PLUGIN))
    {
	int n;

	if (fuseGetDisplayOptionsFromInode (d, inode, &n))
	    fuseAddInode (inode, FUSE_INODE_TYPE_DISPLAY, "allscreen");

	for (s = d->screens; s; s = s->next)
	{
	    if (fuseGetScreenOptionsFromInode (s, inode, &n))
	    {
		sprintf (str, "screen%d", s->screenNum);
		fuseAddInode (inode, FUSE_INODE_TYPE_SCREEN, str);
	    }
	}
    }
    else if (inode->type & (FUSE_INODE_TYPE_DISPLAY | FUSE_INODE_TYPE_SCREEN))
    {
	int nOption;

	option = fuseGetOptionsFromInode (d, inode, &nOption);
	if (option)
	{
	    while (nOption--)
	    {
		fuseAddInode (inode, FUSE_INODE_TYPE_OPTION, option->name);

		option++;
	    }
	}
    }
    else if (inode->type & FUSE_INODE_TYPE_OPTION)
    {
	option = fuseGetOptionFromInode (d, inode);
	if (option)
	{
	    int i;

	    fuseAddInode (inode, FUSE_INODE_TYPE_SHORT_DESC,
			  "short_description");
	    fuseAddInode (inode, FUSE_INODE_TYPE_LONG_DESC,
			  "long_description");
	    fuseAddInode (inode, FUSE_INODE_TYPE_TYPE, "type");

	    switch (option->type) {
	    case CompOptionTypeFloat:
		fuseAddInode (inode, FUSE_INODE_TYPE_PRECISION, "precision");
		/* fall-through */
	    case CompOptionTypeInt:
		fuseAddInode (inode, FUSE_INODE_TYPE_MIN, "min");
		fuseAddInode (inode, FUSE_INODE_TYPE_MAX, "max");
		/* fall-through */
	    case CompOptionTypeBool:
	    case CompOptionTypeColor:
	    case CompOptionTypeMatch:
		fuseAddInode (inode, FUSE_INODE_TYPE_VALUE, "value");
		break;
	    case CompOptionTypeString:
		fuseAddInode (inode, FUSE_INODE_TYPE_VALUE, "value");
		if (option->rest.s.nString)
		    fuseAddInode (inode, FUSE_INODE_TYPE_SELECTION,
				  "selection");
		break;
	    case CompOptionTypeAction:
		fuseAddInode (inode, FUSE_INODE_TYPE_KEY, "key");
		fuseAddInode (inode, FUSE_INODE_TYPE_BUTTON, "button");

		for (i = 0; i < sizeof (edgeName) / sizeof (edgeName[0]); i++)
		    fuseAddInode (inode, FUSE_INODE_TYPE_EDGE, edgeName[i]);

		fuseAddInode (inode, FUSE_INODE_TYPE_EDGE_BUTTON,
			      "edge_button");
		fuseAddInode (inode, FUSE_INODE_TYPE_BELL, "bell");
		break;
	    case CompOptionTypeList:
		fuseAddInode (inode, FUSE_INODE_TYPE_ITEMS, "items");
		fuseAddInode (inode, FUSE_INODE_TYPE_ITEM_COUNT,
			      "number_of_items");
		fuseAddInode (inode, FUSE_INODE_TYPE_ITEM_TYPE, "item_type");

		if (option->value.list.type == CompOptionTypeString)
		{
		    if (option->rest.s.nString)
			fuseAddInode (inode, FUSE_INODE_TYPE_SELECTION,
				      "selection");
		}
	    default:
		break;
	    }
	}
    }
    else if (inode->type & FUSE_INODE_TYPE_ITEMS)
    {
	option = fuseGetOptionFromInode (d, inode->parent);
	if (option && option->type == CompOptionTypeList)
	{
	    FuseInode *c, *next;
	    int	      i, nValue = option->value.list.nValue;

	    for (i = 0; i < option->value.list.nValue; i++)
	    {
		sprintf (str, "value%d", i);
		if (!fuseLookupChild (inode, str))
		    fuseAddInode (inode, FUSE_INODE_TYPE_ITEM_VALUE, str);
	    }

	    for (c = inode->child; c; c = next)
	    {
		next = c->sibling;

		if (sscanf (c->name, "value%d", &i) == 0 || i >= nValue)
		    fuseRemoveInode (inode, c);
	    }
	}
    }
    else if (inode->type & FUSE_INODE_TYPE_SELECTION)
    {
	option = fuseGetOptionFromInode (d, inode->parent);
	if (option)
	{
	    int i;

	    for (i = 0; i < option->rest.s.nString; i++)
	    {
		sprintf (str, "alternative%d", i);
		fuseAddInode (inode, FUSE_INODE_TYPE_ALTERNATIVE, str);
	    }
	}
    }
}

static void
fuseInodeStat (CompDisplay *d,
	       FuseInode   *inode,
	       struct stat *stbuf)
{
    stbuf->st_ino = inode->ino;

    if (inode->type & DIR_MASK)
    {
	stbuf->st_mode  = S_IFDIR | 0755;
	stbuf->st_nlink = 2;
    }
    else
    {
	char *str;

	if (inode->type & WRITE_MASK)
	    stbuf->st_mode = S_IFREG | 0666;
	else
	    stbuf->st_mode = S_IFREG | 0444;

	stbuf->st_nlink = 1;
	stbuf->st_size  = 0;

	str = fuseGetStringFromInode (d, inode);
	if (str)
	{
	    stbuf->st_size = strlen (str);
	    free (str);
	}
    }
}

static Bool
fuseInitValue (CompOptionValue *value,
	       CompOptionType  type,
	       CompOptionValue *src)
{
    switch (type) {
    case CompOptionTypeBool:
	value->b = src->b;
	break;
    case CompOptionTypeInt:
	value->i = src->i;
	break;
    case CompOptionTypeFloat:
	value->f = src->f;
	break;
    case CompOptionTypeString:
	value->s = strdup (src->s);
	break;
    case CompOptionTypeColor:
	memcpy (value->c, src->c, sizeof (*src->c));
	break;
    case CompOptionTypeMatch:
	matchInit (&value->match);
	matchCopy (&value->match, &src->match);
	break;
    default:
	return FALSE;
    }

    return TRUE;
}

static Bool
fuseInitValueFromString (CompOptionValue *value,
			 CompOptionType  type,
			 char		 *str)
{
    switch (type) {
    case CompOptionTypeBool:
	value->b = strcmp (str, "true") ? FALSE : TRUE;
	break;
    case CompOptionTypeInt:
	value->i = atoi (str);
	break;
    case CompOptionTypeFloat:
	value->f = strtod (str, NULL);
	break;
    case CompOptionTypeString:
	value->s = strdup (str);
	break;
    case CompOptionTypeColor:
	if (!stringToColor (str, value->c))
	    return FALSE;
	break;
    case CompOptionTypeMatch:
	matchInit (&value->match);
	matchAddFromString (&value->match, str);
	break;
    default:
	return FALSE;
    }

    return TRUE;
}

static void
fuseFiniValue (CompOptionValue *value,
	       CompOptionType  type)
{
    int i;

    switch (type) {
    case CompOptionTypeString:
	if (value->s)
	    free (value->s);
	break;
    case CompOptionTypeMatch:
	matchFini (&value->match);
	break;
    case CompOptionTypeList:
	if (value->list.nValue)
	{
	    for (i = 0; i < value->list.nValue; i++)
		fuseFiniValue (&value->list.value[i], value->list.type);

	    free (value->list.value);
	}
    default:
	break;
    }
}

static void
fuseSetInodeOptionUsingString (CompDisplay *d,
			       FuseInode   *inode,
			       char	   *str)
{
    CompOption *option;

    option = fuseGetOptionFromInode (d, inode->parent);
    if (option)
    {
	CompOptionValue value;
	CompScreen	*s = NULL;
	char		*plugin = NULL;
	FuseInode	*screenInode = NULL;

	if (inode->type & FUSE_INODE_TYPE_VALUE)
	{
	    if (!fuseInitValueFromString (&value, option->type, str))
		return;

	    screenInode = inode->parent->parent;
	}
	else if (inode->type & ACTION_MASK)
	{
	    value.action = option->value.action;

	    if (inode->type & FUSE_INODE_TYPE_KEY)
	    {
		if (!stringToKeyBinding (d, str, &value.action.key))
		    return;
	    }
	    else if (inode->type & FUSE_INODE_TYPE_BUTTON)
	    {
		if (!stringToButtonBinding (d, str, &value.action.button))
		    return;
	    }
	    else if (inode->type & FUSE_INODE_TYPE_BELL)
	    {
		value.action.bell = strcmp (str, "true") ? FALSE : TRUE;
	    }
	    else if (inode->type & FUSE_INODE_TYPE_EDGE)
	    {
		int i;

		for (i = 0; i < sizeof (edgeName) / sizeof (edgeName[0]); i++)
		{
		    if (strcmp (inode->name, edgeName[i]) == 0)
		    {
			if (strcmp (str, "true") == 0)
			    value.action.edgeMask |= (1 << i);
			else
			    value.action.edgeMask &= ~(1 << i);

			break;
		    }
		}
	    }
	    else if (inode->type & FUSE_INODE_TYPE_EDGE_BUTTON)
	    {
		value.action.edgeButton = atoi (str);
	    }

	    screenInode = inode->parent->parent;
	}
	else if (inode->type & FUSE_INODE_TYPE_ITEM_VALUE)
	{
	    int i, item, nValue = option->value.list.nValue;

	    if (!sscanf (inode->name, "value%d", &item))
		return;

	    if (item >= nValue)
		return;

	    value.list.value = malloc (sizeof (CompOptionValue) * nValue);
	    if (!value.list.value)
		return;

	    value.list.type   = option->value.list.type;
	    value.list.nValue = 0;

	    for (i = 0; i < nValue; i++)
	    {
		if (i == item)
		{
		    if (!fuseInitValueFromString (&value.list.value[i],
						  value.list.type,
						  str))
			break;
		}
		else
		{
		    if (!fuseInitValue (&value.list.value[i],
					value.list.type,
					&option->value.list.value[i]))
			break;
		}

		value.list.nValue++;
	    }

	    /* failed */
	    if (value.list.nValue < nValue)
	    {
		fuseFiniValue (&value, option->type);
		return;
	    }

	    screenInode = inode->parent->parent->parent;
	}

	if (screenInode->type & FUSE_INODE_TYPE_SCREEN)
	{
	    int screenNum = -1;

	    sscanf (screenInode->name, "screen%d", &screenNum);

	    for (s = d->screens; s; s = s->next)
		if (s->screenNum == screenNum)
		    break;
	}

	if (screenInode->parent->type & FUSE_INODE_TYPE_PLUGIN)
	    plugin = screenInode->parent->name;

	if (s)
	{
	    if (plugin)
		(*s->setScreenOptionForPlugin) (s,
						plugin,
						option->name,
						&value);
	    else
		(*s->setScreenOption) (s, option->name, &value);
	}
	else
	{
	    if (plugin)
		(*d->setDisplayOptionForPlugin) (d,
						 plugin,
						 option->name,
						 &value);
	    else
		(*d->setDisplayOption) (d, option->name, &value);
	}

	fuseFiniValue (&value, option->type);
    }
}

static void
compiz_getattr (fuse_req_t	      req,
		fuse_ino_t	      ino,
		struct fuse_file_info *fi)
{
    CompDisplay	*d = (CompDisplay *) fuse_req_userdata (req);
    FuseInode   *inode;

    inode = fuseFindInode (inodes, ino, ~0);
    if (inode)
    {
	struct stat stbuf;

	memset (&stbuf, 0, sizeof (stbuf));

	fuseInodeStat (d, inode, &stbuf);

	fuse_reply_attr (req, &stbuf, 1.0);
    }
    else
    {
	fuse_reply_err (req, ENOENT);
    }
}

static void
compiz_setattr (fuse_req_t	      req,
		fuse_ino_t	      ino,
		struct stat	      *attr,
		int		      to_set,
		struct fuse_file_info *fi)
{
    CompDisplay *d = (CompDisplay *) fuse_req_userdata (req);
    FuseInode   *inode;

    inode = fuseFindInode (inodes, ino, WRITE_MASK);
    if (inode)
    {
	struct stat stbuf;

	if ((to_set & FUSE_SET_ATTR_SIZE) != FUSE_SET_ATTR_SIZE)
	{
	    fuse_reply_err (req, EACCES);
	    return;
	}

	if (attr->st_size != 0)
	{
	    fuse_reply_err (req, EACCES);
	    return;
	}

	inode->flags |= FUSE_INODE_FLAG_TRUNC;

	memset (&stbuf, 0, sizeof (stbuf));

	fuseInodeStat (d, inode, &stbuf);

	fuse_reply_attr (req, &stbuf, 1.0);
    }
    else
    {
	fuse_reply_err (req, ENOENT);
    }
}

static void
compiz_lookup (fuse_req_t req,
	       fuse_ino_t parent,
	       const char *name)
{
    CompDisplay		    *d = (CompDisplay *) fuse_req_userdata (req);
    FuseInode		    *inode;
    struct fuse_entry_param e;

    inode = fuseFindInode (inodes, parent, DIR_MASK);
    if (!inode)
    {
	fuse_reply_err (req, ENOENT);
	return;
    }

    if (!inode->child || !(inode->type & CONST_DIR_MASK))
	fuseUpdateInode (d, inode);

    inode = fuseLookupChild (inode, name);
    if (!inode)
    {
	fuse_reply_err (req, ENOENT);
	return;
    }

    memset (&e, 0, sizeof (e));

    e.attr_timeout  = 1.0;
    e.entry_timeout = 1.0;
    e.ino	    = inode->ino;

    fuseInodeStat (d, inode, &e.attr);

    fuse_reply_entry (req, &e);
}

struct dirbuf {
    char   *p;
    size_t size;
};

static void
dirbuf_add (fuse_req_t	  req,
	    struct dirbuf *b,
	    const char	  *name,
	    fuse_ino_t	  ino)
{
    struct stat stbuf;
    size_t	oldSize = b->size;

    b->size += fuse_add_direntry (req, NULL, 0, name, NULL, 0);
    b->p     = (char *) realloc (b->p, b->size);

    memset (&stbuf, 0, sizeof (stbuf));

    stbuf.st_ino = ino;

    fuse_add_direntry (req, b->p + oldSize, b->size - oldSize, name, &stbuf,
		       b->size);
}

static int
reply_buf_limited (fuse_req_t req,
		   const char *buf,
		   size_t     bufsize,
		   off_t      off,
		   size_t     maxsize)
{
    if (off < bufsize)
	return fuse_reply_buf (req, buf + off, MIN (bufsize - off, maxsize));
    else
	return fuse_reply_buf (req, NULL, 0);
}

static void
compiz_readdir (fuse_req_t	      req,
		fuse_ino_t	      ino,
		size_t		      size,
		off_t		      off,
		struct fuse_file_info *fi)
{
    CompDisplay   *d = (CompDisplay *) fuse_req_userdata (req);
    FuseInode	  *inode, *c;
    struct dirbuf b;

    inode = fuseFindInode (inodes, ino, DIR_MASK);
    if (!inode)
    {
	fuse_reply_err (req, ENOTDIR);
	return;
    }

    memset (&b, 0, sizeof (b));

    dirbuf_add (req, &b, ".", ino);
    dirbuf_add (req, &b, "..", inode->parent ? inode->parent->ino : ino);

    if (!inode->child || !(inode->type & CONST_DIR_MASK))
	fuseUpdateInode (d, inode);

    for (c = inode->child; c; c = c->sibling)
	dirbuf_add (req, &b, c->name, c->ino);

    reply_buf_limited (req, b.p, b.size, off, size);

    free (b.p);
}

static void
compiz_open (fuse_req_t		   req,
	     fuse_ino_t		   ino,
	     struct fuse_file_info *fi)
{
    CompDisplay *d = (CompDisplay *) fuse_req_userdata (req);
    FuseInode   *inode;

    inode = fuseFindInode (inodes, ino, ~0);
    if (!inode)
    {
	fuse_reply_err (req, ENOENT);
	return;
    }

    fi->fh = 0;

    if (inode->type & DIR_MASK)
    {
	fuse_reply_err (req, EISDIR);
    }
    else if (inode->type & WRITE_MASK)
    {
	if ((fi->flags & 3) != O_RDONLY)
	{
	    char *data;

	    if (fi->flags & O_TRUNC)
		data = strdup ("");
	    else
		data = fuseGetStringFromInode (d, inode);

	    if (data)
	    {
		FuseWriteBuffer *wb;

		wb = malloc (sizeof (FuseWriteBuffer));
		if (wb)
		{
		    wb->data  = data;
		    wb->size  = strlen (wb->data);
		    wb->dirty = TRUE;

		    fi->fh = (unsigned long) wb;
		}
		else
		{
		    free (data);
		}
	    }
	}

	fuse_reply_open (req, fi);
    }
    else if ((fi->flags & 3) != O_RDONLY)
    {
	fuse_reply_err (req, EACCES);
    }
    else
    {
	fuse_reply_open (req, fi);
    }
}

static void
compiz_read (fuse_req_t		   req,
	     fuse_ino_t		   ino,
	     size_t		   size,
	     off_t		   off,
	     struct fuse_file_info *fi)
{
    CompDisplay *d = (CompDisplay *) fuse_req_userdata (req);
    FuseInode   *inode;
    char	*str = NULL;

    inode = fuseFindInode (inodes, ino, ~0);
    if (inode)
	str = fuseGetStringFromInode (d, inode);

    if (str)
    {
	reply_buf_limited (req, str, strlen (str), off, size);
	free (str);
    }
    else
    {
	reply_buf_limited (req, NULL, 0, off, size);
    }
}

static void
compiz_write (fuse_req_t	    req,
	      fuse_ino_t	    ino,
	      const char	    *buf,
	      size_t		    size,
	      off_t		    off,
	      struct fuse_file_info *fi)
{
    FuseInode *inode;

    inode = fuseFindInode (inodes, ino, WRITE_MASK);
    if (inode && fi->fh)
    {
	FuseWriteBuffer *wb = (FuseWriteBuffer *) (uintptr_t) fi->fh;

	if (off + size > wb->size)
	{
	    char *data;

	    data = realloc (wb->data, off + size + 1);
	    if (!data)
	    {
		fuse_reply_err (req, ENOBUFS);
		return;
	    }

	    data[off + size] = '\0';

	    wb->data = data;
	    wb->size = off + size;
	}

	memcpy (wb->data + off, buf, size);

	wb->dirty = TRUE;

	fuse_reply_write (req, size);
    }
    else
    {
	fuse_reply_err (req, ENOENT);
    }
}

static void
compiz_release (fuse_req_t	      req,
		fuse_ino_t	      ino,
		struct fuse_file_info *fi)
{
    CompDisplay *d = (CompDisplay *) fuse_req_userdata (req);

    if (fi->fh)
    {
	FuseWriteBuffer *wb = (FuseWriteBuffer *) (uintptr_t) fi->fh;
	FuseInode	*inode;

	inode = fuseFindInode (inodes, ino, WRITE_MASK);
	if (inode && wb->dirty)
	{
	    fuseSetInodeOptionUsingString (d, inode, wb->data);

	    inode->flags &= ~FUSE_INODE_FLAG_TRUNC;
	}

	free (wb->data);
	free (wb);
    }

    fuse_reply_err (req, 0);
}

static void
compiz_fsync (fuse_req_t	    req,
	      fuse_ino_t	    ino,
	      int		    datasync,
	      struct fuse_file_info *fi)
{
    CompDisplay *d = (CompDisplay *) fuse_req_userdata (req);

    if (fi->fh)
    {
	FuseWriteBuffer *wb = (FuseWriteBuffer *) (uintptr_t) fi->fh;
	FuseInode	*inode;

	inode = fuseFindInode (inodes, ino, WRITE_MASK);
	if (inode && wb->dirty)
	{
	    fuseSetInodeOptionUsingString (d, inode, wb->data);

	    inode->flags &= ~FUSE_INODE_FLAG_TRUNC;

	    wb->dirty = FALSE;
	}
    }

    fuse_reply_err (req, 0);
}

static struct fuse_lowlevel_ops compiz_ll_oper = {
    .lookup  = compiz_lookup,
    .getattr = compiz_getattr,
    .setattr = compiz_setattr,
    .readdir = compiz_readdir,
    .open    = compiz_open,
    .read    = compiz_read,
    .write   = compiz_write,
    .release = compiz_release,
    .fsync   = compiz_fsync
};

static void
fuseUnmount (CompDisplay *d)
{
    FUSE_DISPLAY (d);

    if (fd->watchFdHandle)
    {
	compRemoveWatchFd (fd->watchFdHandle);
	fd->watchFdHandle = 0;
    }

    if (fd->mountPoint)
    {
	/* unmount will destroy the channel */
	fuse_unmount (fd->mountPoint, fd->channel);
	free (fd->mountPoint);
	fd->mountPoint = NULL;
	fd->channel = NULL;
    }

    if (fd->buffer)
    {
	free (fd->buffer);
	fd->buffer = NULL;
    }
}

static Bool
fuseProcessMessages (void *data)
{
    CompDisplay	     *d = (CompDisplay *) data;
    struct fuse_chan *channel;
    size_t	     bufferSize;
    int		     res = 0;

    FUSE_DISPLAY (d);

    channel    = fuse_session_next_chan (fd->session, NULL);
    bufferSize = fuse_chan_bufsize (channel);

    if (fuse_session_exited (fd->session))
	return FALSE;

    for (;;)
    {
	struct fuse_chan *tmpch = channel;

	res = fuse_chan_recv (&tmpch, fd->buffer, bufferSize);
	if (res == -EINTR)
	    continue;

	if (res > 0)
	    fuse_session_process (fd->session, fd->buffer, res, tmpch);

	break;
    }

    return TRUE;
}

static void
fuseMount (CompDisplay *d)
{
    char	     *mountPoint;
    struct fuse_args args = FUSE_ARGS_INIT (0, NULL);

    FUSE_DISPLAY (d);

    mountPoint = strdup (fd->opt[FUSE_DISPLAY_OPTION_MOUNT_POINT].value.s);
    if (!mountPoint)
	return;

    fuse_opt_add_arg (&args, "");
    fuse_opt_add_arg (&args, "-o");
    fuse_opt_add_arg (&args, "allow_root");

    fd->channel = fuse_mount (mountPoint, &args);
    if (!fd->channel)
    {
	fuse_opt_free_args (&args);
	free (mountPoint);
	return;
    }

    fuse_opt_free_args (&args);

    fd->buffer = malloc (fuse_chan_bufsize (fd->channel));
    if (!fd->buffer)
    {
	fuse_unmount (mountPoint, fd->channel);
	free (mountPoint);
	fd->channel = NULL;
	return;
    }

    fd->mountPoint = mountPoint;

    fuse_session_add_chan (fd->session, fd->channel);

    fd->watchFdHandle = compAddWatchFd (fuse_chan_fd (fd->channel),
					POLLIN | POLLPRI | POLLHUP | POLLERR,
					fuseProcessMessages,
					d);
}

static CompOption *
fuseGetDisplayOptions (CompPlugin  *plugin,
		       CompDisplay *display,
		       int	   *count)
{
    FUSE_DISPLAY (display);

    *count = NUM_OPTIONS (fd);
    return fd->opt;
}

static Bool
fuseSetDisplayOption (CompPlugin      *plugin,
		      CompDisplay     *display,
		      char	      *name,
		      CompOptionValue *value)
{
    CompOption *o;
    int	       index;

    FUSE_DISPLAY (display);

    o = compFindOption (fd->opt, NUM_OPTIONS (fd), name, &index);
    if (!o)
	return FALSE;

    switch (index) {
    case FUSE_DISPLAY_OPTION_MOUNT_POINT:
	if (compSetStringOption (o, value))
	{
	    fuseUnmount (display);
	    fuseMount (display);
	    return TRUE;
	}
    default:
	break;
    }

    return FALSE;
}

static void
fuseDisplayInitOptions (FuseDisplay *fd)
{
    CompOption *o;

    o = &fd->opt[FUSE_DISPLAY_OPTION_MOUNT_POINT];
    o->name	      = "mount_point";
    o->shortDesc      = N_("Mount Point");
    o->longDesc	      = N_("Mount point");
    o->type	      = CompOptionTypeString;
    o->value.s	      = strdup (FUSE_MOUNT_POINT_DEFAULT);
    o->rest.s.string  = NULL;
    o->rest.s.nString = 0;
}

static Bool
fuseInitDisplay (CompPlugin  *p,
		 CompDisplay *d)
{
    FuseDisplay	     *fd;
    struct sigaction sa;

    memset (&sa, 0, sizeof (struct sigaction));

    sa.sa_handler = SIG_IGN;
    sigemptyset (&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction (SIGPIPE, &sa, NULL) == -1)
	return FALSE;

    fd = malloc (sizeof (FuseDisplay));
    if (!fd)
	return FALSE;

    fuseDisplayInitOptions (fd);

    fd->session = fuse_lowlevel_new (NULL,
				     &compiz_ll_oper, sizeof (compiz_ll_oper),
				     (void *) d);
    if (!fd->session)
    {
	free (fd);
	return FALSE;
    }

    fd->watchFdHandle = 0;
    fd->channel	      = NULL;
    fd->buffer	      = NULL;
    fd->mountPoint    = NULL;

    d->privates[displayPrivateIndex].ptr = fd;

    fuseMount (d);

    return TRUE;
}

static void
fuseFiniDisplay (CompPlugin  *p,
		 CompDisplay *d)
{
    FUSE_DISPLAY (d);

    fuseUnmount (d);

    fuse_session_destroy (fd->session);

    free (fd->opt[FUSE_DISPLAY_OPTION_MOUNT_POINT].value.s);
    free (fd);
}

static Bool
fuseInit (CompPlugin *p)
{
    inodes = fuseAddInode (NULL, FUSE_INODE_TYPE_ROOT, ".");
    if (!inodes)
	return FALSE;

    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if (displayPrivateIndex < 0)
    {
	fuseRemoveInode (NULL, inodes);
	return FALSE;
    }

    return TRUE;
}

static void
fuseFini (CompPlugin *p)
{
    if (displayPrivateIndex >= 0)
	freeDisplayPrivateIndex (displayPrivateIndex);

    fuseRemoveInode (NULL, inodes);
}

static int
fuseGetVersion (CompPlugin *plugin,
		int	   version)
{
    return ABIVERSION;
}

CompPluginVTable fuseVTable = {
    "fs",
    N_("Userspace File System"),
    N_("Userspace file system"),
    fuseGetVersion,
    fuseInit,
    fuseFini,
    fuseInitDisplay,
    fuseFiniDisplay,
    0, /* InitScreen */
    0, /* FiniScreen */
    0, /* InitWindow */
    0, /* FiniWindow */
    fuseGetDisplayOptions,
    fuseSetDisplayOption,
    0, /* GetScreenOptions */
    0, /* SetScreenOption */
    0, /* Deps */
    0, /* nDeps */
    0, /* Features */
    0  /* nFeatures */
};

CompPluginVTable *
getCompPluginInfo (void)
{
    return &fuseVTable;
}
