/*
 * Compiz configuration system library
 *
 * Copyright (C) 2012 Canonical Ltd.
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
 *
 * Authored By:
 * Sam Spilsbury <sam.spilsbury@canonical.com>
 */

#include "ccs_settings_upgrade_internal.h"
#include <ccs.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

typedef Bool (*CCSProcessTokenizedUpgradeFileFunc) (const char *name,
						    int        length,
						    const char *tokenOne,
						    const char *tokenTwo,
						    const char *tokenThree,
						    int        foundNumber,
						    void       *userData);

static Bool
isUpgrade (const char *name,
	   int        length,
	   const char *tokenOne,
	   const char *tokenTwo,
	   const char *tokenThree,
	   int        foundNumber,
	   void       *userData)
{
    static const char         *UPGRADE = "upgrade";
    static const unsigned int UPGRADE_STR_LEN = 7;

    if (strncmp (tokenThree, UPGRADE, UPGRADE_STR_LEN))
	return FALSE;

    return TRUE;
}

typedef struct _FillDomainNumAndProfileData
{
    char **domain;
    char **profile;
    unsigned int  *num;
} FillDomainNumAndProfileData;

static int
fillDomainNumAndProfile (const char *name,
			 int        length,
			 const char *tokenOne,
			 const char *tokenTwo,
			 const char *tokenThree,
			 int        foundNumber,
			 void       *userData)
{
    FillDomainNumAndProfileData *data = (FillDomainNumAndProfileData *) userData;

    *data->domain = strndup (name, length - (strlen (tokenOne) + 1));

    /* profile.n.upgrade */
    *data->profile = strndup (tokenOne, strlen (tokenOne) - (strlen (tokenTwo) + 1));
    *data->num = foundNumber;

    return 1;
}

static Bool
ccsDetokenizeUpgradeDomainAndExecuteUserFunc (const char			 *name,
					      CCSProcessTokenizedUpgradeFileFunc func,
					      void				 *userData)
{
    int length = strlen (name);
    const char *tok = name;
    Bool       success = FALSE;

    /* Keep removing domains and other bits
     * until we hit a number that we can parse */
    while (tok)
    {
	long int numTmp = 0;
	char *nexttok = strchr (tok, '.');
	char *nextnexttok = NULL;
	char *end = NULL;
	char *bit = NULL;

	if (!nexttok)
	    return FALSE;

	nexttok++;
	nextnexttok = strchr (nexttok, '.');

	if (!nextnexttok)
	    return FALSE;

	nextnexttok++;
	bit = strndup (nexttok, strlen (nexttok) - (strlen (nextnexttok) + 1));

	/* FIXME: That means that the number can't be a zero */
	errno = 0;
	numTmp = strtol (bit, &end, 10);

	if (!(errno != 0 && numTmp == 0) &&
	    end != bit)
	{
	    if ((*func) (name,
			 length,
			 tok,
			 nexttok,
			 nextnexttok,
			 numTmp,
			 userData))
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

Bool
ccsUpgradeGetDomainNumAndProfile (const char   *name,
				  char         **domain,
				  unsigned int *num,
				  char         **profile)
{
    FillDomainNumAndProfileData data =
    {
	domain,
	profile,
	num
    };

    return ccsDetokenizeUpgradeDomainAndExecuteUserFunc (name, fillDomainNumAndProfile, (void *) &data);
}

int
ccsUpgradeNameFilter (const char *name)
{
    Bool result = FALSE;
    int length = strlen (name);

    if (length < 7)
	return 0;

    result = ccsDetokenizeUpgradeDomainAndExecuteUserFunc (name, isUpgrade, NULL);

    if (result)
	return 1;

    return 0;
}

void
ccsUpgradeClearValues (CCSSettingList clearSettings)
{
    CCSSettingList sl = clearSettings;

    while (sl)
    {
	CCSSetting *tempSetting = (CCSSetting *) sl->data;
	CCSSetting *setting;
	CCSPlugin  *plugin = ccsSettingGetParent (tempSetting);
	const char *name = ccsSettingGetName (tempSetting);

	setting = ccsFindSetting (plugin, name);

	if (setting)
	{
	    if (ccsSettingGetType (setting) != TypeList)
	    {
		if (ccsSettingGetValue (setting) == ccsSettingGetValue (tempSetting))
		{
		    ccsDebug ("Resetting %s to default", ccsSettingGetName ((CCSSetting *) sl->data));
		    ccsResetToDefault (setting, TRUE);
		}
		else
		{
		    ccsDebug ("Skipping processing of %s", ccsSettingGetName ((CCSSetting *) sl->data));
		}
	    }
	    else
	    {
		unsigned int count = 0;
		/* Try and remove any specified items from the list */
		CCSSettingValueList l = ccsSettingGetValue (tempSetting)->value.asList;
		CCSSettingValueList nl = ccsCopyList (ccsSettingGetValue (setting)->value.asList, setting);

		while (l)
		{
		    CCSSettingValueList olv = nl;

		    while (olv)
		    {
			CCSSettingValue *lv = (CCSSettingValue *) l->data;
			CCSSettingValue *olvv = (CCSSettingValue *) olv->data;

			if (ccsCheckValueEq (lv, olvv))
			    break;

			olv = olv->next;
		    }

		    if (olv)
		    {
			count++;
			nl = ccsSettingValueListRemove (nl, olv->data, TRUE);
		    }

		    l = l->next;
		}

		ccsDebug ("Removed %i items from %s", count, ccsSettingGetName (setting));
		ccsSetList (setting, nl, TRUE);

	    }
	}

	sl = sl->next;
    }
}

void
ccsUpgradeAddValues (CCSSettingList addSettings)
{
    CCSSettingList sl = addSettings;

    while (sl)
    {
	CCSSetting *tempSetting = (CCSSetting *) sl->data;
	CCSSetting *setting;
	CCSPlugin  *plugin = ccsSettingGetParent (tempSetting);
	const char *name = ccsSettingGetName (tempSetting);

	setting = ccsFindSetting (plugin, name);

	if (setting)
	{
	    ccsDebug ("Overriding value %s", ccsSettingGetName ((CCSSetting *) sl->data));
	    if (ccsSettingGetType (setting) != TypeList)
		ccsSetValue (setting, ccsSettingGetValue (tempSetting), TRUE);
	    else
	    {
		unsigned int count = 0;
		/* Try and apppend any new items to the list */
		CCSSettingValueList l = ccsSettingGetValue (tempSetting)->value.asList;
		CCSSettingValueList nl = ccsCopyList (ccsSettingGetValue (setting)->value.asList, setting);

		while (l)
		{
		    CCSSettingValueList olv = nl;

		    while (olv)
		    {
			CCSSettingValue *lv = (CCSSettingValue *) l->data;
			CCSSettingValue *olvv = (CCSSettingValue *) olv->data;

			if (ccsCheckValueEq (lv, olvv))
			    break;

			olv = olv->next;
		    }

		    if (!olv)
		    {
			count++;
			nl = ccsSettingValueListAppend (nl, l->data);
		    }

		    l = l->next;
		}

		ccsDebug ("Appending %i items to %s", count, ccsSettingGetName (setting));
		ccsSetList (setting, nl, TRUE);
	    }
	}
	else
	{
	    ccsDebug ("Value %s not found!", ccsSettingGetName ((CCSSetting *) sl->data));
	}

	sl = sl->next;
    }
}

void
ccsUpgradeReplaceValues (CCSSettingList replaceFromValueSettings,
			 CCSSettingList replaceToValueSettings)
{
    CCSSettingList sl = replaceFromValueSettings;

    while (sl)
    {
	CCSSetting *tempSetting = (CCSSetting *) sl->data;
	CCSSetting *setting;
	CCSPlugin  *plugin = ccsSettingGetParent (tempSetting);
	const char *name = ccsSettingGetName (tempSetting);

	setting = ccsFindSetting (plugin, name);

	if (setting)
	{
	    if (ccsSettingGetValue (setting) == ccsSettingGetValue (tempSetting))
	    {
		CCSSettingList rl = replaceToValueSettings;

		while (rl)
		{
		    CCSSetting *rsetting = (CCSSetting *) rl->data;

		    if (strcmp (ccsSettingGetName (rsetting), ccsSettingGetName (setting)) == 0)
		    {
			ccsDebug ("Matched and replaced %s", ccsSettingGetName (setting));
			ccsSetValue (setting, ccsSettingGetValue (rsetting), TRUE);
			break;
		    }

		    rl = rl->next;
		}
	    }
	    else
	    {
		ccsDebug ("Skipping processing of %s", ccsSettingGetName ((CCSSetting *) sl->data));
	    }
	}

	sl = sl->next;
    }
}
