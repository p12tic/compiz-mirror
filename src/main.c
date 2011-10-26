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

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <libintl.h>
#include <dlfcn.h>
#include <dirent.h>
#include <math.h>
#include <errno.h>

#include <ccs.h>

#include "ccs-private.h"
#include "iniparser.h"

Bool basicMetadata = FALSE;

void
ccsSetBasicMetadata (Bool value)
{
    basicMetadata = value;
}

static void
initGeneralOptions (CCSContext * context)
{
    char *val;

    if (ccsReadConfig (OptionBackend, &val))
    {
	ccsSetBackend (context, val);
	free (val);
    }
    else
	ccsSetBackend (context, "ini");

    if (ccsReadConfig (OptionProfile, &val))
    {
	ccsSetProfile (context, val);
	free (val);
    }
    else
	ccsSetProfile (context, "");

    if (ccsReadConfig (OptionIntegration, &val))
    {
	ccsSetIntegrationEnabled (context, !strcasecmp (val, "true"));
	free (val);
    }
    else
	ccsSetIntegrationEnabled (context, TRUE);

    if (ccsReadConfig (OptionAutoSort, &val))
    {
	ccsSetPluginListAutoSort (context, !strcasecmp (val, "true"));
	free (val);
    }
    else
	ccsSetPluginListAutoSort (context, TRUE);
}

static void
configChangeNotify (unsigned int watchId, void *closure)
{
    CCSContext *context = (CCSContext *) closure;

    initGeneralOptions (context);
    ccsReadSettings (context);
}

CCSContext *
ccsEmptyContextNew (unsigned int screenNum)
{
    CCSContext *context;

    context = calloc (1, sizeof (CCSContext));
    if (!context)
	return NULL;

    context->ccsPrivate = calloc (1, sizeof (CCSContextPrivate));
    if (!context->ccsPrivate)
    {
	free (context);
	return NULL;
    }

    CONTEXT_PRIV (context);

    context->screenNum = screenNum;

    initGeneralOptions (context);
    cPrivate->configWatchId = ccsAddConfigWatch (context, configChangeNotify);

    if (cPrivate->backend)
	D (D_NORMAL, "Backend     : %s\n", cPrivate->backend->vTable->name);
	D (D_NORMAL, "Integration : %s\n", cPrivate->deIntegration ? "true" : "false");
	D (D_NORMAL, "Profile     : %s\n",
	    (cPrivate->profile && strlen (cPrivate->profile)) ?
	    cPrivate->profile : "default");

    return context;
}

static void
ccsSetActivePluginList (CCSContext * context, CCSStringList list)
{
    CCSPluginList l;
    CCSPlugin     *plugin;

    for (l = context->plugins; l; l = l->next)
    {
	PLUGIN_PRIV (l->data);
	pPrivate->active = FALSE;
    }

    for (; list; list = list->next)
    {
	plugin = ccsFindPlugin (context, ((CCSString *)list->data)->value);

	if (plugin)
	{
	    PLUGIN_PRIV (plugin);
	    pPrivate->active = TRUE;
	}
    }

    /* core plugin is always active */
    plugin = ccsFindPlugin (context, "core");
    if (plugin)
    {
	PLUGIN_PRIV (plugin);
	pPrivate->active = TRUE;
    }
}

CCSContext *
ccsContextNew (unsigned int screenNum)
{
    CCSPlugin  *p;
    CCSContext *context = ccsEmptyContextNew (screenNum);
    if (!context)
	return NULL;

    ccsLoadPlugins (context);

    /* Do settings upgrades */
    ccsCheckForSettingsUpgrade (context);

    p = ccsFindPlugin (context, "core");
    if (p)
    {
	CCSSetting    *s;

	ccsLoadPluginSettings (p);

	/* initialize plugin->active values */
	s = ccsFindSetting (p, "active_plugins");
	if (s)
	{
	    CCSStringList       list;
	    CCSSettingValueList vl;

	    ccsGetList (s, &vl);
	    list = ccsGetStringListFromValueList (vl);
	    ccsSetActivePluginList (context, list);
	    ccsStringListFree (list, TRUE);
	}
    }

    return context;
}

CCSPlugin *
ccsFindPlugin (CCSContext * context, const char *name)
{
    if (!name)
	name = "";

    CCSPluginList l = context->plugins;
    while (l)
    {
	if (!strcmp (l->data->name, name))
	    return l->data;

	l = l->next;
    }

    return NULL;
}

CCSSetting *
ccsFindSetting (CCSPlugin * plugin, const char *name)
{
    if (!plugin)
	return NULL;

    PLUGIN_PRIV (plugin);

    if (!name)
	name = "";

    if (!pPrivate->loaded)
	ccsLoadPluginSettings (plugin);

    CCSSettingList l = pPrivate->settings;

    while (l)
    {
	if (!strcmp (l->data->name, name))
	    return l->data;

	l = l->next;
    }

    return NULL;
}

Bool
ccsPluginIsActive (CCSContext * context, char *name)
{
    CCSPlugin *plugin;

    plugin = ccsFindPlugin (context, name);
    if (!plugin)
	return FALSE;

    PLUGIN_PRIV (plugin);

    return pPrivate->active;
}


static void
subGroupAdd (CCSSetting * setting, CCSGroup * group)
{
    CCSSubGroupList l = group->subGroups;
    CCSSubGroup     *subGroup;

    while (l)
    {
	if (!strcmp (l->data->name, setting->subGroup))
	{
	    l->data->settings = ccsSettingListAppend (l->data->settings,
						      setting);
	    return;
	}

	l = l->next;
    }

    subGroup = calloc (1, sizeof (CCSSubGroup));
    subGroup->refCount = 1;
    if (subGroup)
    {
	group->subGroups = ccsSubGroupListAppend (group->subGroups, subGroup);
	subGroup->name = strdup (setting->subGroup);
	subGroup->settings = ccsSettingListAppend (subGroup->settings, setting);
    }
}

static void
groupAdd (CCSSetting * setting, CCSPluginPrivate * p)
{
    CCSGroupList l = p->groups;
    CCSGroup     *group;

    while (l)
    {
	if (!strcmp (l->data->name, setting->group))
	{
	    subGroupAdd (setting, l->data);
	    return;
	}

	l = l->next;
    }

    group = calloc (1, sizeof (CCSGroup));
    if (group)
    {
	group->refCount = 1;
    	p->groups = ccsGroupListAppend (p->groups, group);
	group->name = strdup (setting->group);
	subGroupAdd (setting, group);
    }
}

void
collateGroups (CCSPluginPrivate * p)
{
    CCSSettingList l = p->settings;

    while (l)
    {
	groupAdd (l->data, p);
	l = l->next;
    }
}

void
ccsFreeContext (CCSContext * c)
{
    if (!c)
	return;

    CONTEXT_PRIV (c);

    if (cPrivate->profile)
	free (cPrivate->profile);

    if (cPrivate->configWatchId)
	ccsRemoveFileWatch (cPrivate->configWatchId);

    if (c->changedSettings)
	ccsSettingListFree (c->changedSettings, FALSE);

    if (c->ccsPrivate)
	free (c->ccsPrivate);

    ccsPluginListFree (c->plugins, TRUE);

    free (c);
}

void
ccsFreePlugin (CCSPlugin * p)
{
    if (!p)
	return;

    free (p->name);
    free (p->shortDesc);
    free (p->longDesc);
    free (p->hints);
    free (p->category);

    ccsStringListFree (p->loadAfter, TRUE);
    ccsStringListFree (p->loadBefore, TRUE);
    ccsStringListFree (p->requiresPlugin, TRUE);
    ccsStringListFree (p->conflictPlugin, TRUE);
    ccsStringListFree (p->conflictFeature, TRUE);
    ccsStringListFree (p->providesFeature, TRUE);
    ccsStringListFree (p->requiresFeature, TRUE);

    PLUGIN_PRIV (p);

    ccsSettingListFree (pPrivate->settings, TRUE);
    ccsGroupListFree (pPrivate->groups, TRUE);
    ccsStrExtensionListFree (pPrivate->stringExtensions, TRUE);

    if (pPrivate->xmlFile)
	free (pPrivate->xmlFile);

    if (pPrivate->xmlPath)
	free (pPrivate->xmlPath);

#ifdef USE_PROTOBUF
    if (pPrivate->pbFilePath)
	free (pPrivate->pbFilePath);
#endif

    free (pPrivate);
    free (p);
}

void
ccsFreeSetting (CCSSetting * s)
{
    if (!s)
	return;

    free (s->name);
    free (s->shortDesc);
    free (s->longDesc);
    free (s->group);
    free (s->subGroup);
    free (s->hints);

    switch (s->type)
    {
    case TypeInt:
	ccsIntDescListFree (s->info.forInt.desc, TRUE);
	break;
    case TypeString:
	ccsStrRestrictionListFree (s->info.forString.restriction, TRUE);
	break;
    case TypeList:
	if (s->info.forList.listType == TypeInt)
	    ccsIntDescListFree (s->info.forList.listInfo->
				forInt.desc, TRUE);
	free (s->info.forList.listInfo);
	//ccsSettingValueListFree (s->value->value.asList, TRUE);
	break;
    default:
	break;
    }

    if (&s->defaultValue != s->value)
    {
	ccsFreeSettingValue (s->value);
    }

    ccsFreeSettingValue (&s->defaultValue);
    free (s);
}

void
ccsFreeGroup (CCSGroup * g)
{
    if (!g)
	return;

    free (g->name);
    ccsSubGroupListFree (g->subGroups, TRUE);
    free (g);
}

void
ccsFreeSubGroup (CCSSubGroup * s)
{
    if (!s)
	return;

    free (s->name);
    ccsSettingListFree (s->settings, FALSE);
    free (s);
}

void
ccsFreeSettingValue (CCSSettingValue * v)
{
    if (!v)
	return;

    if (!v->parent)
	return;

    CCSSettingType type = v->parent->type;

    if (v->isListChild)
	type = v->parent->info.forList.listType;

    switch (type)
    {
    case TypeString:
	free (v->value.asString);
	break;
    case TypeMatch:
	free (v->value.asMatch);
	break;
    case TypeList:
	if (!v->isListChild)
	    ccsSettingValueListFree (v->value.asList, TRUE);
	break;
    default:
	break;
    }

    if (v != &v->parent->defaultValue)
	free (v);
}

void
ccsFreePluginConflict (CCSPluginConflict * c)
{
    if (!c)
	return;

    free (c->value);

    ccsPluginListFree (c->plugins, FALSE);

    free (c);
}

void
ccsFreeBackendInfo (CCSBackendInfo * b)
{
    if (!b)
	return;

    if (b->name)
	free (b->name);

    if (b->shortDesc)
	free (b->shortDesc);

    if (b->longDesc)
	free (b->longDesc);

    free (b);
}

void
ccsFreeIntDesc (CCSIntDesc * i)
{
    if (!i)
	return;

    if (i->name)
	free (i->name);

    free (i);
}

void
ccsFreeStrRestriction (CCSStrRestriction * r)
{
    if (!r)
	return;

    if (r->name)
	free (r->name);

    if (r->value)
	free (r->value);

    free (r);
}

void
ccsFreeStrExtension (CCSStrExtension *e)
{
    if (!e)
	return;

    if (e->basePlugin)
	free (e->basePlugin);

    ccsStringListFree (e->baseSettings, TRUE);
    ccsStrRestrictionListFree (e->restriction, TRUE);

    free (e);
}

void
ccsFreeString (CCSString *str)
{
    if (str->value)
	free (str->value);
    
    free (str);
}

#define CCSREF(type,dtype) \
	void ccs##type##Ref (dtype *d)  \
	{ \
	    d->refCount++; \
	} \
	void ccs##type##Unref (dtype *d) \
	{ \
	    d->refCount--; \
	    if (d->refCount == 0) \
		ccsFree##type (d); \
	} \

CCSREF (Plugin, CCSPlugin)
CCSREF (Setting, CCSSetting)
CCSREF (String, CCSString)
CCSREF (Group, CCSGroup)
CCSREF (SubGroup, CCSSubGroup)
CCSREF (SettingValue, CCSSettingValue)
CCSREF (PluginConflict, CCSPluginConflict)
CCSREF (BackendInfo, CCSBackendInfo)
CCSREF (IntDesc, CCSIntDesc)
CCSREF (StrRestriction, CCSStrRestriction)
CCSREF (StrExtension, CCSStrExtension)

static void *
openBackend (char *backend)
{
    char *home = getenv ("HOME");
    void *dlhand = NULL;
    char *dlname = NULL;
    char *err = NULL;

    if (home && strlen (home))
    {
	asprintf (&dlname, "%s/.compizconfig/backends/lib%s.so", 
		  home, backend);
	dlerror ();
	dlhand = dlopen (dlname, RTLD_NOW | RTLD_NODELETE | RTLD_LOCAL);
	err = dlerror ();
    }

    if (!dlhand)
    {
        if (dlname) {
	        free (dlname);
        }
	asprintf (&dlname, "%s/compizconfig/backends/lib%s.so", 
		  LIBDIR, backend);
	dlhand = dlopen (dlname, RTLD_NOW | RTLD_NODELETE | RTLD_LOCAL);
	err = dlerror ();
    }

    free (dlname);

    if (err)
    {
	fprintf (stderr, "libcompizconfig: dlopen: %s\n", err);
    }

    return dlhand;
}

Bool
ccsSetBackend (CCSContext * context, char *name)
{
    Bool fallbackMode = FALSE;
    CONTEXT_PRIV (context);

    if (cPrivate->backend)
    {
	/* no action needed if the backend is the same */

	if (strcmp (cPrivate->backend->vTable->name, name) == 0)
	    return TRUE;

	if (cPrivate->backend->vTable->backendFini)
	    cPrivate->backend->vTable->backendFini (context);

	dlclose (cPrivate->backend->dlhand);
	free (cPrivate->backend);
	cPrivate->backend = NULL;
    }

    void *dlhand = openBackend (name);
    if (!dlhand)
    {
	fallbackMode = TRUE;
	name = "ini";
	dlhand = openBackend (name);
    }

    if (!dlhand)
	return FALSE;

    BackendGetInfoProc getInfo = dlsym (dlhand, "getBackendInfo");
    if (!getInfo)
    {
	dlclose (dlhand);
	return FALSE;
    }

    CCSBackendVTable *vt = getInfo ();
    if (!vt)
    {
	dlclose (dlhand);
	return FALSE;
    }

    cPrivate->backend = calloc (1, sizeof (CCSBackend));
    if (!cPrivate->backend)
    {
	dlclose (dlhand);
	return FALSE;
    }
    cPrivate->backend->dlhand = dlhand;
    cPrivate->backend->vTable = vt;

    if (cPrivate->backend->vTable->backendInit)
	cPrivate->backend->vTable->backendInit (context);

    ccsDisableFileWatch (cPrivate->configWatchId);
    if (!fallbackMode)
	ccsWriteConfig (OptionBackend, name);
    ccsEnableFileWatch (cPrivate->configWatchId);

    return TRUE;
}


static Bool
ccsCompareLists (CCSSettingValueList l1, CCSSettingValueList l2,
		 CCSSettingListInfo info)
{
    while (l1 && l2)
    {
	switch (info.listType)
	{
	case TypeInt:
	    if (l1->data->value.asInt != l2->data->value.asInt)
		return FALSE;
	    break;
	case TypeBool:
	    if (l1->data->value.asBool != l2->data->value.asBool)
		return FALSE;
	    break;
	case TypeFloat:
	    if (l1->data->value.asFloat != l2->data->value.asFloat)
		return FALSE;
	    break;
	case TypeString:
	    if (strcmp (l1->data->value.asString, l2->data->value.asString))
		return FALSE;
	    break;
	case TypeMatch:
	    if (strcmp (l1->data->value.asMatch, l2->data->value.asMatch))
		return FALSE;
	    break;
	case TypeKey:
	    if (!ccsIsEqualKey
		(l1->data->value.asKey, l2->data->value.asKey))
		return FALSE;
	    break;
	case TypeButton:
	    if (!ccsIsEqualButton
		(l1->data->value.asButton, l2->data->value.asButton))
		return FALSE;
	    break;
	case TypeEdge:
	    if (l1->data->value.asEdge != l2->data->value.asEdge)
		return FALSE;
	    break;
	case TypeBell:
	    if (l1->data->value.asBell != l2->data->value.asBell)
		return FALSE;
	    break;
	case TypeColor:
	    if (!ccsIsEqualColor
		(l1->data->value.asColor, l2->data->value.asColor))
		return FALSE;
	    break;
	default:
	    return FALSE;
	    break;
	}

	l1 = l1->next;
	l2 = l2->next;
    }

    if ((!l1 && l2) || (l1 && !l2))
	return FALSE;

    return TRUE;
}

static void
copyInfo (CCSSettingInfo *from, CCSSettingInfo *to, CCSSettingType type)
{	
    memcpy (from, to, sizeof (CCSSettingInfo));

    switch (type)
    {
	case TypeInt:
	{
	    CCSIntDescList idl = from->forInt.desc;

	    to->forInt = from->forInt;
	    to->forInt.desc = NULL;
	    
	    while (idl)
	    {
		CCSIntDesc *id = malloc (sizeof (CCSIntDesc));

		if (!idl->data)
		{
		    free (id);
		    idl = idl->next;
		    continue;
		}

		memcpy (id, idl->data, sizeof (CCSIntDesc));
		
		id->name = strdup (idl->data->name);
		id->refCount = 1;

	        to->forInt.desc = ccsIntDescListAppend (to->forInt.desc, id);
		
		idl = idl->next;
	    }
	    
	    break;
	}
	case TypeFloat:
	    to->forFloat = from->forFloat;
	    break;
	case TypeString:
	{
	    CCSStrRestrictionList srl = from->forString.restriction;

	    to->forString = from->forString;
	    to->forString.restriction = NULL;
	    
	    while (srl)
	    {
		CCSStrRestriction *sr = malloc (sizeof (CCSStrRestriction));

		if (!srl->data)
		{
		    srl = srl->next;
		    free (sr);
		    continue;
		}

		memcpy (sr, srl->data, sizeof (CCSStrRestriction));
		
		sr->name = strdup (srl->data->name);
		sr->value = strdup (srl->data->value);
		sr->refCount = 1;

	        to->forString.restriction = ccsStrRestrictionListAppend (to->forString.restriction, sr);
		
		srl = srl->next;
	    }
	    
	    break;
	}
	case TypeList:
	{
	    to->forList.listInfo = calloc (1, sizeof (CCSSettingInfo));
	    
	    copyInfo (from->forList.listInfo, to->forList.listInfo, from->forList.listType);

	    break;
	}
	case TypeAction:
	    to->forAction.internal = from->forAction.internal;
	    break;
	default:
	    break;
    }
}

static void
copyValue (CCSSettingValue * from, CCSSettingValue * to)
{
    memcpy (to, from, sizeof (CCSSettingValue));
    CCSSettingType type = from->parent->type;

    if (from->isListChild)
	type = from->parent->info.forList.listType;

    switch (type)
    {
    case TypeString:
	to->value.asString = strdup (from->value.asString);
	break;
    case TypeMatch:
	to->value.asMatch = strdup (from->value.asMatch);
	break;
    case TypeList:
	to->value.asList = NULL;
	CCSSettingValueList l = from->value.asList;
	while (l)
	{
	    CCSSettingValue *value = calloc (1, sizeof (CCSSettingValue));
	    if (!value)
		break;

	    copyValue (l->data, value);
	    value->refCount = 1;
	    to->value.asList = ccsSettingValueListAppend (to->value.asList,
							  value);
	    l = l->next;
	}
	break;
    default:
	break;
    }
}

static void
copySetting (CCSSetting *from, CCSSetting *to)
{    
    memcpy (to, from, sizeof (CCSSetting));

    if (from->name)
	to->name = strdup (from->name);
    if (from->shortDesc)
	to->shortDesc = strdup (from->shortDesc);
    if (from->longDesc)
	to->longDesc = strdup (from->longDesc);
    if (from->group)
	to->group = strdup (from->group);
    if (from->subGroup)
	to->subGroup = strdup (from->subGroup);
    if (from->hints)
	to->hints = strdup (from->hints);
    if (from->value)
    {
	to->value = malloc (sizeof (CCSSettingValue));
	
	if (!from->value)
	    return;

	copyValue (from->value, to->value);

	to->value->refCount = 1;	
	to->value->parent = to;
    }

    copyValue (&from->defaultValue, &to->defaultValue);
    copyInfo (&from->info, &to->info, from->type);

    to->defaultValue.parent = to;
    to->privatePtr = NULL;
    
    to->refCount = 1;
}

static void
copyFromDefault (CCSSetting * setting)
{
    CCSSettingValue *value;

    if (setting->value != &setting->defaultValue)
	ccsFreeSettingValue (setting->value);

    value = calloc (1, sizeof (CCSSettingValue));
    if (!value)
    {
	setting->value = &setting->defaultValue;
	setting->isDefault = TRUE;
	return;
    }
    
    value->refCount = 1;

    copyValue (&setting->defaultValue, value);
    setting->value = value;
    setting->isDefault = FALSE;
}

void
ccsResetToDefault (CCSSetting * setting, Bool processChanged)
{
    if (setting->value != &setting->defaultValue)
    {
	ccsFreeSettingValue (setting->value);

	if (processChanged)
	    setting->parent->context->changedSettings =
		ccsSettingListAppend (setting->parent->context->changedSettings,
				       setting);
    }

    setting->value = &setting->defaultValue;
    setting->isDefault = TRUE;
}

Bool
ccsCheckValueEq (CCSSettingValue *rhs, CCSSettingValue *lhs)
{
    CCSSettingType type;

    if (rhs->parent->type != lhs->parent->type)
    {
	D (D_FULL, "[WARNING] Attempted to check equality between mismatched types!\n");
	return FALSE;
    }

    if (rhs->isListChild)
	type = rhs->parent->info.forList.listType;
    else
	type = rhs->parent->type;
    
    switch (type)
    {
	case TypeInt:
	    return lhs->value.asInt == rhs->value.asInt;
	case TypeBool:
	    return lhs->value.asBool == rhs->value.asBool;
	case TypeFloat:
	    return lhs->value.asFloat == rhs->value.asFloat;
	case TypeMatch:
	    return strcmp (lhs->value.asMatch, rhs->value.asMatch) == 0;
	case TypeString:
	    return strcmp (lhs->value.asString, rhs->value.asString) == 0;
	case TypeColor:
	    return ccsIsEqualColor (lhs->value.asColor, rhs->value.asColor);
	case TypeKey:
	    return ccsIsEqualKey (lhs->value.asKey, rhs->value.asKey);
	case TypeButton:
	    return ccsIsEqualButton (lhs->value.asButton, rhs->value.asButton);
	case TypeEdge:
	    return lhs->value.asEdge == rhs->value.asEdge;
	case TypeBell:
	    return lhs->value.asBell == rhs->value.asBell;
	case TypeAction:
	    D (D_FULL, "[WARNING] Actions are not comparable!\n");
	    return FALSE;
	case TypeList:
	{
	    return ccsCompareLists (lhs->value.asList, rhs->value.asList,
				    lhs->parent->info.forList);
	
	}
	default:
	    break;
    }
    
    D (D_FULL, "[WARNING] Failed to process type %i\n", lhs->parent->type);
    return FALSE;
}

Bool
ccsSetInt (CCSSetting * setting, int data, Bool processChanged)
{
    if (setting->type != TypeInt)
	return FALSE;

    if (setting->isDefault && (setting->defaultValue.value.asInt == data))
	return TRUE;

    if (!setting->isDefault && (setting->defaultValue.value.asInt == data))
    {
	ccsResetToDefault (setting, processChanged);
	return TRUE;
    }

    if (setting->value->value.asInt == data)
	return TRUE;

    if ((data < setting->info.forInt.min) ||
	 (data > setting->info.forInt.max))
	return FALSE;

    if (setting->isDefault)
	copyFromDefault (setting);

    setting->value->value.asInt = data;

    if (processChanged)
	setting->parent->context->changedSettings =
	    ccsSettingListAppend (setting->parent->context->changedSettings,
				  setting);

    return TRUE;
}

Bool
ccsSetFloat (CCSSetting * setting, float data, Bool processChanged)
{
    if (setting->type != TypeFloat)
	return FALSE;

    if (setting->isDefault && (setting->defaultValue.value.asFloat == data))
	return TRUE;

    if (!setting->isDefault && (setting->defaultValue.value.asFloat == data))
    {
	ccsResetToDefault (setting, processChanged);
	return TRUE;
    }

    /* allow the values to differ a tiny bit because of
       possible rounding / precision issues */
    if (fabs (setting->value->value.asFloat - data) < 1e-5)
	return TRUE;

    if ((data < setting->info.forFloat.min) ||
	 (data > setting->info.forFloat.max))
	return FALSE;

    if (setting->isDefault)
	copyFromDefault (setting);

    setting->value->value.asFloat = data;

    if (processChanged)
	setting->parent->context->changedSettings =
	    ccsSettingListAppend (setting->parent->context->changedSettings,
				  setting);

    return TRUE;
}

Bool
ccsSetBool (CCSSetting * setting, Bool data, Bool processChanged)
{
    if (setting->type != TypeBool)
	return FALSE;

    if (setting->isDefault
	&& ((setting->defaultValue.value.asBool && data)
	     || (!setting->defaultValue.value.asBool && !data)))
	return TRUE;

    if (!setting->isDefault
	&& ((setting->defaultValue.value.asBool && data)
	     || (!setting->defaultValue.value.asBool && !data)))
    {
	ccsResetToDefault (setting, processChanged);
	return TRUE;
    }

    if ((setting->value->value.asBool && data)
	 || (!setting->value->value.asBool && !data))
	return TRUE;

    if (setting->isDefault)
	copyFromDefault (setting);

    setting->value->value.asBool = data;

    if (processChanged)
	setting->parent->context->changedSettings =
	    ccsSettingListAppend (setting->parent->context->changedSettings,
				  setting);

    return TRUE;
}

Bool
ccsSetString (CCSSetting * setting, const char *data, Bool processChanged)
{
    if (setting->type != TypeString)
	return FALSE;

    if (!data)
	return FALSE;

    Bool isDefault = strcmp (setting->defaultValue.value.asString, data) == 0;

    if (setting->isDefault && isDefault)
	return TRUE;

    if (!setting->isDefault && isDefault)
    {
	ccsResetToDefault (setting, processChanged);
	return TRUE;
    }

    if (!strcmp (setting->value->value.asString, data))
	return TRUE;

    if (setting->isDefault)
	copyFromDefault (setting);

    free (setting->value->value.asString);

    setting->value->value.asString = strdup (data);

    if (processChanged)
	setting->parent->context->changedSettings =
		ccsSettingListAppend (setting->parent->context->changedSettings,
				      setting);

    return TRUE;
}

Bool
ccsSetColor (CCSSetting * setting, CCSSettingColorValue data, Bool processChanged)
{
    if (setting->type != TypeColor)
	return FALSE;

    CCSSettingColorValue defValue = setting->defaultValue.value.asColor;

    Bool isDefault = ccsIsEqualColor (defValue, data);

    if (setting->isDefault && isDefault)
	return TRUE;

    if (!setting->isDefault && isDefault)
    {
	ccsResetToDefault (setting, processChanged);
	return TRUE;
    }

    if (ccsIsEqualColor (setting->value->value.asColor, data))
	return TRUE;

    if (setting->isDefault)
	copyFromDefault (setting);

    setting->value->value.asColor = data;

    if (processChanged)
	setting->parent->context->changedSettings =
	    ccsSettingListAppend (setting->parent->context->changedSettings,
				  setting);

    return TRUE;
}

Bool
ccsSetMatch (CCSSetting * setting, const char *data, Bool processChanged)
{
    if (setting->type != TypeMatch)
	return FALSE;

    if (!data)
	return FALSE;

    Bool isDefault = strcmp (setting->defaultValue.value.asMatch, data) == 0;

    if (setting->isDefault && isDefault)
	return TRUE;

    if (!setting->isDefault && isDefault)
    {
	ccsResetToDefault (setting, processChanged);
	return TRUE;
    }

    if (!strcmp (setting->value->value.asMatch, data))
	return TRUE;

    if (setting->isDefault)
	copyFromDefault (setting);

    free (setting->value->value.asMatch);

    setting->value->value.asMatch = strdup (data);

    if (processChanged)
	setting->parent->context->changedSettings =
	    ccsSettingListAppend (setting->parent->context->changedSettings,
				  setting);

    return TRUE;
}

Bool
ccsSetKey (CCSSetting * setting, CCSSettingKeyValue data, Bool processChanged)
{
    if (setting->type != TypeKey)
	return FALSE;

    CCSSettingKeyValue defValue = setting->defaultValue.value.asKey;

    Bool isDefault = ccsIsEqualKey (data, defValue);

    if (setting->isDefault && isDefault)
	return TRUE;

    if (!setting->isDefault && isDefault)
    {
	ccsResetToDefault (setting, processChanged);
	return TRUE;
    }

    if (ccsIsEqualKey (setting->value->value.asKey, data))
	return TRUE;

    if (setting->isDefault)
	copyFromDefault (setting);

    setting->value->value.asKey.keysym = data.keysym;
    setting->value->value.asKey.keyModMask = data.keyModMask;

    if (processChanged)
	setting->parent->context->changedSettings =
	    ccsSettingListAppend (setting->parent->context->changedSettings,
				  setting);

    return TRUE;
}

Bool
ccsSetButton (CCSSetting * setting, CCSSettingButtonValue data, Bool processChanged)
{
    if (setting->type != TypeButton)
	return FALSE;

    CCSSettingButtonValue defValue = setting->defaultValue.value.asButton;

    Bool isDefault = ccsIsEqualButton (data, defValue);

    if (setting->isDefault && isDefault)
	return TRUE;

    if (!setting->isDefault && isDefault)
    {
	ccsResetToDefault (setting, processChanged);
	return TRUE;
    }

    if (ccsIsEqualButton (setting->value->value.asButton, data))
	return TRUE;

    if (setting->isDefault)
	copyFromDefault (setting);

    setting->value->value.asButton.button = data.button;
    setting->value->value.asButton.buttonModMask = data.buttonModMask;
    setting->value->value.asButton.edgeMask = data.edgeMask;

    if (processChanged)
	setting->parent->context->changedSettings =
	    ccsSettingListAppend (setting->parent->context->changedSettings,
				  setting);

    return TRUE;
}

Bool
ccsSetEdge (CCSSetting * setting, unsigned int data, Bool processChanged)
{
    if (setting->type != TypeEdge)
	return FALSE;

    Bool isDefault = (data == setting->defaultValue.value.asEdge);

    if (setting->isDefault && isDefault)
	return TRUE;

    if (!setting->isDefault && isDefault)
    {
	ccsResetToDefault (setting, processChanged);
	return TRUE;
    }

    if (setting->value->value.asEdge == data)
	return TRUE;

    if (setting->isDefault)
	copyFromDefault (setting);

    setting->value->value.asEdge = data;

    if (processChanged)
	setting->parent->context->changedSettings =
	     ccsSettingListAppend (setting->parent->context->changedSettings,
				   setting);

    return TRUE;
}

Bool
ccsSetBell (CCSSetting * setting, Bool data, Bool processChanged)
{
    if (setting->type != TypeBell)
	return FALSE;

    Bool isDefault = (data == setting->defaultValue.value.asBool);

    if (setting->isDefault && isDefault)
	return TRUE;

    if (!setting->isDefault && isDefault)
    {
	ccsResetToDefault (setting, processChanged);
	return TRUE;
    }

    if (setting->value->value.asBell == data)
	return TRUE;

    if (setting->isDefault)
	copyFromDefault (setting);

    setting->value->value.asBell = data;

    if (processChanged)
	setting->parent->context->changedSettings =
	    ccsSettingListAppend (setting->parent->context->changedSettings,
				  setting);

    return TRUE;
}

static CCSSettingValueList
ccsCopyList (CCSSettingValueList l1, CCSSetting * setting)
{
    CCSSettingValueList l2 = NULL;

    while (l1)
    {
	CCSSettingValue *value = calloc (1, sizeof (CCSSettingValue));
	if (!value)
	    return l2;

	value->refCount = 1;
	value->parent = setting;
	value->isListChild = TRUE;

	switch (setting->info.forList.listType)
	{
	case TypeInt:
	    value->value.asInt = l1->data->value.asInt;
	    break;
	case TypeBool:
	    value->value.asBool = l1->data->value.asBool;
	    break;
	case TypeFloat:
	    value->value.asFloat = l1->data->value.asFloat;
	    break;
	case TypeString:
	    value->value.asString = strdup (l1->data->value.asString);
	    break;
	case TypeMatch:
	    value->value.asMatch = strdup (l1->data->value.asMatch);
	    break;
	case TypeKey:
	    memcpy (&value->value.asKey, &l1->data->value.asKey,
		    sizeof (CCSSettingKeyValue));
	    break;
	case TypeButton:
	    memcpy (&value->value.asButton, &l1->data->value.asButton,
		    sizeof (CCSSettingButtonValue));
	    break;
	case TypeEdge:
	    value->value.asEdge = l1->data->value.asEdge;
	    break;
	case TypeBell:
	    value->value.asBell = l1->data->value.asBell;
	    break;
	case TypeColor:
	    memcpy (&value->value.asColor, &l1->data->value.asColor,
		    sizeof (CCSSettingColorValue));
	    break;
	default:
	  /* FIXME If l2 != NULL, we leak l2 */
	    free (value);
	    return FALSE;
	    break;
	}

	l2 = ccsSettingValueListAppend (l2, value);
	l1 = l1->next;
    }

    return l2;
}

Bool
ccsSetList (CCSSetting * setting, CCSSettingValueList data, Bool processChanged)
{
    if (setting->type != TypeList)
	return FALSE;

    Bool isDefault = ccsCompareLists (setting->defaultValue.value.asList, data,
				      setting->info.forList);

    /* Don't need to worry about default values
     * when processChanged is off since use of that
     * API wants direct access to ths list for
     * temporary storage */
    if (!processChanged)
    {
	if (setting->isDefault && isDefault)
	{
	    return TRUE;
	}

	if (!setting->isDefault && isDefault)
	{
	    ccsResetToDefault (setting, processChanged);
	    return TRUE;
	}
    }

    if (ccsCompareLists (setting->value->value.asList, data,
			 setting->info.forList))
    {
	return TRUE;
    }

    if (setting->isDefault)
	copyFromDefault (setting);

    ccsSettingValueListFree (setting->value->value.asList, TRUE);

    setting->value->value.asList = ccsCopyList (data, setting);

    if ((strcmp (setting->name, "active_plugins") == 0) &&
	(strcmp (setting->parent->name, "core") == 0) && processChanged)
    {
	CCSStringList list;

	list = ccsGetStringListFromValueList (setting->value->value.asList);
	ccsSetActivePluginList (setting->parent->context, list);
	ccsStringListFree (list, TRUE);
    }

    if (processChanged)
	setting->parent->context->changedSettings =
	    ccsSettingListAppend (setting->parent->context->changedSettings,
				  setting);

    return TRUE;
}

Bool
ccsSetValue (CCSSetting * setting, CCSSettingValue * data, Bool processChanged)
{
    switch (setting->type)
    {
    case TypeInt:
	return ccsSetInt (setting, data->value.asInt, processChanged);
	break;
    case TypeFloat:
	return ccsSetFloat (setting, data->value.asFloat, processChanged);
	break;
    case TypeBool:
	return ccsSetBool (setting, data->value.asBool, processChanged);
	break;
    case TypeColor:
	return ccsSetColor (setting, data->value.asColor, processChanged);
	break;
    case TypeString:
	return ccsSetString (setting, data->value.asString, processChanged);
	break;
    case TypeMatch:
	return ccsSetMatch (setting, data->value.asMatch, processChanged);
	break;
    case TypeKey:
	return ccsSetKey (setting, data->value.asKey, processChanged);
	break;
    case TypeButton:
	return ccsSetButton (setting, data->value.asButton, processChanged);
	break;
    case TypeEdge:
	return ccsSetEdge (setting, data->value.asEdge, processChanged);
	break;
    case TypeBell:
	return ccsSetBell (setting, data->value.asBell, processChanged);
	break;
    case TypeList:
	return ccsSetList (setting, data->value.asList, processChanged);
    default:
	break;
    }

    return FALSE;
}

Bool
ccsGetInt (CCSSetting * setting, int *data)
{
    if (setting->type != TypeInt)
	return FALSE;

    *data = setting->value->value.asInt;
    return TRUE;
}

Bool
ccsGetFloat (CCSSetting * setting, float *data)
{
    if (setting->type != TypeFloat)
	return FALSE;

    *data = setting->value->value.asFloat;
    return TRUE;
}

Bool
ccsGetBool (CCSSetting * setting, Bool * data)
{
    if (setting->type != TypeBool)
	return FALSE;

    *data = setting->value->value.asBool;
    return TRUE;
}

Bool
ccsGetString (CCSSetting * setting, char **data)
{
    if (setting->type != TypeString)
	return FALSE;

    *data = setting->value->value.asString;
    return TRUE;
}

Bool
ccsGetColor (CCSSetting * setting, CCSSettingColorValue * data)
{
    if (setting->type != TypeColor)
	return TRUE;

    *data = setting->value->value.asColor;
    return TRUE;
}

Bool
ccsGetMatch (CCSSetting * setting, char **data)
{
    if (setting->type != TypeMatch)
	return FALSE;

    *data = setting->value->value.asMatch;
    return TRUE;
}

Bool
ccsGetKey (CCSSetting * setting, CCSSettingKeyValue * data)
{
    if (setting->type != TypeKey)
	return FALSE;

    *data = setting->value->value.asKey;
    return TRUE;
}

Bool
ccsGetButton (CCSSetting * setting, CCSSettingButtonValue * data)
{
    if (setting->type != TypeButton)
	return FALSE;

    *data = setting->value->value.asButton;
    return TRUE;
}

Bool
ccsGetEdge (CCSSetting * setting, unsigned int * data)
{
    if (setting->type != TypeEdge)
	return FALSE;

    *data = setting->value->value.asEdge;
    return TRUE;
}

Bool
ccsGetBell (CCSSetting * setting, Bool * data)
{
    if (setting->type != TypeBell)
	return FALSE;

    *data = setting->value->value.asBell;
    return TRUE;
}

Bool
ccsGetList (CCSSetting * setting, CCSSettingValueList * data)
{
    if (setting->type != TypeList)
	return FALSE;

    *data = setting->value->value.asList;
    return TRUE;
}

void
ccsContextDestroy (CCSContext * context)
{
    if (!context)
	return;

    CONTEXT_PRIV (context);

    if (cPrivate->backend)
    {
	if (cPrivate->backend->vTable->backendFini)
	    cPrivate->backend->vTable->backendFini (context);

	dlclose (cPrivate->backend->dlhand);
	free (cPrivate->backend);
	cPrivate->backend = NULL;
    }

    ccsFreeContext (context);
}

CCSPluginList
ccsGetActivePluginList (CCSContext * context)
{
    CCSPluginList rv = NULL;
    CCSPluginList l = context->plugins;

    while (l)
    {
	PLUGIN_PRIV (l->data);
	if (pPrivate->active && strcmp (l->data->name, "ccp"))
	{
	    rv = ccsPluginListAppend (rv, l->data);
	}

	l = l->next;
    }

    return rv;
}

static CCSPlugin *
findPluginInList (CCSPluginList list, char *name)
{
    if (!name || !strlen (name))
	return NULL;

    while (list)
    {
	if (!strcmp (list->data->name, name))
	    return list->data;

	list = list->next;
    }

    return NULL;
}

typedef struct _PluginSortHelper
{
    CCSPlugin *plugin;
    CCSPluginList after;
} PluginSortHelper;

CCSStringList
ccsGetSortedPluginStringList (CCSContext * context)
{
    CCSPluginList ap = ccsGetActivePluginList (context);
    CCSPluginList list;
    CCSPlugin *p = NULL;
    CCSString *strCore = calloc (1, sizeof (CCSString));

    strCore->value = strdup ("core");
    strCore->refCount = 1;

    CCSStringList rv = ccsStringListAppend (NULL, strCore);
    PluginSortHelper *ph = NULL;

    p = findPluginInList (ap, "core");
    if (p)
	ap = ccsPluginListRemove (ap, p, FALSE);

    int len = ccsPluginListLength (ap);
    if (len == 0)
    {
	ccsStringListFree (rv, TRUE);
	return NULL;
    }
    int i, j;
    /* TODO: conflict handling */

    PluginSortHelper *plugins = calloc (1, len * sizeof (PluginSortHelper));
    if (!plugins)
    {
	ccsStringListFree (rv, TRUE);
	return NULL;
    }

    for (i = 0, list = ap; i < len; i++, list = list->next)
    {
	plugins[i].plugin = list->data;
	plugins[i].after = NULL;
    }

    for (i = 0; i < len; i++)
    {
	CCSStringList l = plugins[i].plugin->loadAfter;
	while (l)
	{
	    p = findPluginInList (ap, ((CCSString *)l->data)->value);

	    if (p && !ccsPluginListFind (plugins[i].after, p))
		plugins[i].after = ccsPluginListAppend (plugins[i].after, p);

	    l = l->next;
	}

	l = plugins[i].plugin->requiresPlugin;
	while (l)
	{
	    Bool found = FALSE;
	    p = findPluginInList (ap, ((CCSString *)l->data)->value);

	    CCSStringList l2 = plugins[i].plugin->loadBefore;
	    while (l2)
	    {
		if (strcmp (((CCSString *)l2->data)->value,
			    ((CCSString *)l->data)->value) == 0)
		    found = TRUE;
		l2 = l2->next;
	    }
	    
	    if (p && !ccsPluginListFind (plugins[i].after, p) && !found)
		plugins[i].after = ccsPluginListAppend (plugins[i].after, p);

	    l = l->next;
	}

	l = plugins[i].plugin->loadBefore;
	while (l)
	{
	    p = findPluginInList (ap, ((CCSString *)l->data)->value);

	    if (p)
	    {
		ph = NULL;

		for (j = 0; j < len; j++)
		    if (p == plugins[j].plugin)
			ph = &plugins[j];

		if (ph && !ccsPluginListFind (ph->after, plugins[i].plugin))
		    ph->after = ccsPluginListAppend (ph->after,
						     plugins[i].plugin);
	    }

	    l = l->next;
	}
    }

    ccsPluginListFree (ap, FALSE);

    Bool error = FALSE;
    int removed = 0;
    Bool found;

    while (!error && removed < len)
    {
	found = FALSE;

	for (i = 0; i < len; i++)
	{
	    CCSString *strPluginName;		
		
	    if (!plugins[i].plugin)
		continue;
	    if (plugins[i].after)
		continue;

	    /* This is a special case to ensure that bench is the last plugin */
	    if (len - removed > 1 &&
		strcmp (plugins[i].plugin->name, "bench") == 0)
		continue;

	    found = TRUE;
	    removed++;
	    p = plugins[i].plugin;
	    plugins[i].plugin = NULL;

	    for (j = 0; j < len; j++)
		plugins[j].after =
		    ccsPluginListRemove (plugins[j].after, p, FALSE);

	    strPluginName = calloc (1, sizeof (CCSString));
	    
	    strPluginName->value = strdup (p->name);
	    strPluginName->refCount = 1;

	    rv = ccsStringListAppend (rv, strPluginName);
	}

	if (!found)
	    error = TRUE;
    }

    if (error)
    {
	fprintf (stderr,
		 "libcompizconfig: unable to generate sorted plugin list\n");

	for (i = 0; i < len; i++)
	{
	    ccsPluginListFree (plugins[i].after, FALSE);
	}

	ccsStringListFree (rv, TRUE);

	rv = NULL;
    }

    free (plugins);

    return rv;
}

char *
ccsGetBackend (CCSContext * context)
{
    if (!context)
	return NULL;

    CONTEXT_PRIV (context);

    if (!cPrivate->backend)
	return NULL;

    return cPrivate->backend->vTable->name;
}

Bool
ccsGetIntegrationEnabled (CCSContext * context)
{
    if (!context)
	return FALSE;

    CONTEXT_PRIV (context);

    return cPrivate->deIntegration;
}

char *
ccsGetProfile (CCSContext * context)
{
    if (!context)
	return NULL;

    CONTEXT_PRIV (context);

    return cPrivate->profile;
}

Bool
ccsGetPluginListAutoSort (CCSContext * context)
{
    if (!context)
	return FALSE;

    CONTEXT_PRIV (context);

    return cPrivate->pluginListAutoSort;
}

void
ccsSetIntegrationEnabled (CCSContext * context, Bool value)
{
    CONTEXT_PRIV (context);

    /* no action required if nothing changed */
    if ((!cPrivate->deIntegration && !value) ||
	 (cPrivate->deIntegration && value))
	return;

    cPrivate->deIntegration = value;

    ccsDisableFileWatch (cPrivate->configWatchId);
    ccsWriteConfig (OptionIntegration, (value) ? "true" : "false");
    ccsEnableFileWatch (cPrivate->configWatchId);
}

static void
ccsWriteAutoSortedPluginList (CCSContext *context)
{
    CCSStringList list;
    CCSPlugin     *p;

    list = ccsGetSortedPluginStringList (context);
    p    = ccsFindPlugin (context, "core");
    if (p)
    {
	CCSSetting *s;

	s = ccsFindSetting (p, "active_plugins");
	if (s)
	{
	    CCSSettingValueList vl;

	    vl = ccsGetValueListFromStringList (list, s);
	    ccsSetList (s, vl, TRUE);
	    ccsSettingValueListFree (vl, TRUE);
	    ccsWriteChangedSettings (context);
	}
    }
    ccsStringListFree (list, TRUE);
}

void
ccsSetPluginListAutoSort (CCSContext * context, Bool value)
{
    CONTEXT_PRIV (context);

    /* no action required if nothing changed */
    if ((!cPrivate->pluginListAutoSort && !value) ||
	 (cPrivate->pluginListAutoSort && value))
	return;

    cPrivate->pluginListAutoSort = value;

    ccsDisableFileWatch (cPrivate->configWatchId);
    ccsWriteConfig (OptionAutoSort, (value) ? "true" : "false");
    ccsEnableFileWatch (cPrivate->configWatchId);

    if (value)
	ccsWriteAutoSortedPluginList (context);
}

void
ccsSetProfile (CCSContext * context, char *name)
{
    if (!name)
	name = "";

    CONTEXT_PRIV (context);

    /* no action required if profile stays the same */
    if (cPrivate->profile && (strcmp (cPrivate->profile, name) == 0))
	return;

    if (cPrivate->profile)
	free (cPrivate->profile);

    cPrivate->profile = strdup (name);

    ccsDisableFileWatch (cPrivate->configWatchId);
    ccsWriteConfig (OptionProfile, cPrivate->profile);
    ccsEnableFileWatch (cPrivate->configWatchId);
}

void
ccsProcessEvents (CCSContext * context, unsigned int flags)
{
    if (!context)
	return;

    CONTEXT_PRIV (context);

    ccsCheckFileWatches ();

    if (cPrivate->backend && cPrivate->backend->vTable->executeEvents)
	(*cPrivate->backend->vTable->executeEvents) (flags);
}

void
ccsReadSettings (CCSContext * context)
{
    if (!context)
	return;
    
    CONTEXT_PRIV (context);
    
    if (!cPrivate->backend)
	return;

    if (!cPrivate->backend->vTable->readSetting)
	return;

    if (cPrivate->backend->vTable->readInit)
	if (!(*cPrivate->backend->vTable->readInit) (context))
	    return;

    CCSPluginList pl = context->plugins;
    while (pl)
    {
	PLUGIN_PRIV (pl->data);
	CCSSettingList sl = pPrivate->settings;

	while (sl)
	{
	    (*cPrivate->backend->vTable->readSetting) (context, sl->data);
	    sl = sl->next;
	}

	pl = pl->next;
    }

    if (cPrivate->backend->vTable->readDone)
	(*cPrivate->backend->vTable->readDone) (context);
}

void
ccsReadPluginSettings (CCSPlugin * plugin)
{
    if (!plugin || !plugin->context)
	return;

    CONTEXT_PRIV (plugin->context);

    if (!cPrivate->backend)
	return;

    if (!cPrivate->backend->vTable->readSetting)
	return;

    if (cPrivate->backend->vTable->readInit)
	if (!(*cPrivate->backend->vTable->readInit) (plugin->context))
	    return;

    PLUGIN_PRIV (plugin);

    CCSSettingList sl = pPrivate->settings;
    while (sl)
    {
	(*cPrivate->backend->vTable->readSetting) (plugin->context, sl->data);
	sl = sl->next;
    }

    if (cPrivate->backend->vTable->readDone)
	(*cPrivate->backend->vTable->readDone) (plugin->context);
}

void
ccsWriteSettings (CCSContext * context)
{
    if (!context)
	return;

    CONTEXT_PRIV (context);

    if (!cPrivate->backend)
	return;

    if (!cPrivate->backend->vTable->writeSetting)
	return;

    if (cPrivate->backend->vTable->writeInit)
	if (!(*cPrivate->backend->vTable->writeInit) (context))
	    return;

    CCSPluginList pl = context->plugins;
    while (pl)
    {
	PLUGIN_PRIV (pl->data);
	CCSSettingList sl = pPrivate->settings;

	while (sl)
	{
	    (*cPrivate->backend->vTable->writeSetting) (context, sl->data);
	    sl = sl->next;
	}

	pl = pl->next;
    }

    if (cPrivate->backend->vTable->writeDone)
	(*cPrivate->backend->vTable->writeDone) (context);

    context->changedSettings =
	ccsSettingListFree (context->changedSettings, FALSE);
}

void
ccsWriteChangedSettings (CCSContext * context)
{
    if (!context)
	return;

    CONTEXT_PRIV (context);
    
    if (!cPrivate->backend)
	return;

    if (!cPrivate->backend->vTable->writeSetting)
	return;

    if (cPrivate->backend->vTable->writeInit)
	if (!(*cPrivate->backend->vTable->writeInit) (context))
	    return;

    if (ccsSettingListLength (context->changedSettings))
    {
	CCSSettingList l = context->changedSettings;

	while (l)
	{
	    (*cPrivate->backend->vTable->writeSetting) (context, l->data);
	    l = l->next;
	}
    }

    if (cPrivate->backend->vTable->writeDone)
	(*cPrivate->backend->vTable->writeDone) (context);

    context->changedSettings =
	ccsSettingListFree (context->changedSettings, FALSE);
}

Bool
ccsIsEqualColor (CCSSettingColorValue c1, CCSSettingColorValue c2)
{
    if (c1.color.red == c2.color.red     &&
	c1.color.green == c2.color.green &&
	c1.color.blue == c2.color.blue   &&
	c1.color.alpha == c2.color.alpha)
    {
	return TRUE;
    }

    return FALSE;
}

Bool
ccsIsEqualKey (CCSSettingKeyValue c1, CCSSettingKeyValue c2)
{
    if (c1.keysym == c2.keysym && c1.keyModMask == c2.keyModMask)
	return TRUE;

    return FALSE;
}

Bool
ccsIsEqualButton (CCSSettingButtonValue c1, CCSSettingButtonValue c2)
{
    if (c1.button == c2.button               &&
	c1.buttonModMask == c2.buttonModMask &&
	c1.edgeMask == c2.edgeMask)
	return TRUE;

    return FALSE;
}

Bool
ccsPluginSetActive (CCSPlugin * plugin, Bool value)
{
    if (!plugin)
	return FALSE;

    PLUGIN_PRIV (plugin);
    CONTEXT_PRIV (plugin->context);

    pPrivate->active = value;

    if (cPrivate->pluginListAutoSort)
	ccsWriteAutoSortedPluginList (plugin->context);

    return TRUE;
}

CCSPluginConflictList
ccsCanEnablePlugin (CCSContext * context, CCSPlugin * plugin)
{
    CCSPluginConflictList list = NULL;
    CCSPluginList pl, pl2;
    CCSStringList sl;

    /* look if the plugin to be loaded requires a plugin not present */
    sl = plugin->requiresPlugin;

    while (sl)
    {
	if (!ccsFindPlugin (context, ((CCSString *)sl->data)->value))
	{
	    CCSPluginConflict *conflict = calloc (1,
						  sizeof (CCSPluginConflict));
	    if (conflict)
	    {
		conflict->refCount = 1;
		conflict->value = strdup (((CCSString *)sl->data)->value);
		conflict->type = ConflictPluginError;
		conflict->plugins = NULL;
		list = ccsPluginConflictListAppend (list, conflict);
	    }
	}
	else if (!ccsPluginIsActive (context, ((CCSString *)sl->data)->value))
	{
	    /* we've not seen a matching plugin */
	    CCSPluginConflict *conflict = calloc (1,
						  sizeof (CCSPluginConflict));
	    if (conflict)
	    {
		conflict->refCount = 1;
		conflict->value = strdup (((CCSString *)sl->data)->value);
		conflict->type = ConflictRequiresPlugin;
		conflict->plugins =
		    ccsPluginListAppend (conflict->plugins,
			    		 ccsFindPlugin (context, ((CCSString *)sl->data)->value));
		list = ccsPluginConflictListAppend (list, conflict);
	    }
	}

	sl = sl->next;
    }

    /* look if the new plugin wants a non-present feature */
    sl = plugin->requiresFeature;

    while (sl)
    {
	pl = context->plugins;
	pl2 = NULL;

	while (pl)
	{
	    CCSStringList featureList = pl->data->providesFeature;

	    while (featureList)
	    {
		if (strcmp (((CCSString *) sl->data)->value, ((CCSString *)featureList->data)->value) == 0)
		{
		    pl2 = ccsPluginListAppend (pl2, pl->data);
		    break;
		}
		featureList = featureList->next;
	    }

	    pl = pl->next;
	}

	pl = pl2;

	while (pl)
	{
	    if (ccsPluginIsActive (context, pl->data->name))
	    {
		ccsPluginListFree (pl2, FALSE);
		break;
	    }
	    pl = pl->next;
	}

	if (!pl)
	{
	    /* no plugin provides that feature */
	    CCSPluginConflict *conflict = calloc (1,
						  sizeof (CCSPluginConflict));

	    if (conflict)
	    {
		conflict->refCount = 1;
		conflict->value = strdup (((CCSString *)sl->data)->value);
		conflict->type = ConflictRequiresFeature;
		conflict->plugins = pl2;

		list = ccsPluginConflictListAppend (list, conflict);
	    }
	}

	sl = sl->next;
    }

    /* look if another plugin provides the same feature */
    sl = plugin->providesFeature;
    while (sl)
    {
	pl = context->plugins;
	CCSPluginConflict *conflict = NULL;

	while (pl)
	{
	    if (ccsPluginIsActive (context, pl->data->name))
	    {
		CCSStringList featureList = pl->data->providesFeature;

		while (featureList)
		{
		    if (strcmp (((CCSString *)sl->data)->value, ((CCSString *)featureList->data)->value) == 0)
		    {
			if (!conflict)
			{
			    conflict = calloc (1, sizeof (CCSPluginConflict));
			    if (conflict)
			    {
				conflict->refCount = 1;
				conflict->value = strdup (((CCSString *)sl->data)->value);
				conflict->type = ConflictFeature;
			    }
			}
			if (conflict)
			    conflict->plugins =
				ccsPluginListAppend (conflict->plugins,
						     pl->data);
		    }
		    featureList = featureList->next;
		}
	    }
	    pl = pl->next;
	}

	if (conflict)
	    list = ccsPluginConflictListAppend (list, conflict);

	sl = sl->next;
    }

    /* look if another plugin provides a conflicting feature*/
    sl = plugin->conflictFeature;
    while (sl)
    {
	pl = context->plugins;
	CCSPluginConflict *conflict = NULL;

	while (pl)
	{
	    if (ccsPluginIsActive (context, pl->data->name))
	    {
		CCSStringList featureList = pl->data->providesFeature;
		while (featureList)
		{
		    if (strcmp (((CCSString *)sl->data)->value, ((CCSString *)featureList->data)->value) == 0)
		    {
			if (!conflict)
			{
			    conflict = calloc (1, sizeof (CCSPluginConflict));
			    if (conflict)
			    {
				conflict->refCount = 1;
				conflict->value = strdup (((CCSString *)sl->data)->value);
				conflict->type = ConflictFeature;
			    }
			}
			if (conflict)
			    conflict->plugins =
				ccsPluginListAppend (conflict->plugins,
						     pl->data);
		    }
		    featureList = featureList->next;
		}
	    }
	    pl = pl->next;
	}

	if (conflict)
	    list = ccsPluginConflictListAppend (list, conflict);

	sl = sl->next;
    }

    /* look if the plugin to be loaded conflict with a loaded plugin  */
    sl = plugin->conflictPlugin;

    while (sl)
    {
	if (ccsPluginIsActive (context, ((CCSString *)sl->data)->value))
	{
	    CCSPluginConflict *conflict = calloc (1,
						  sizeof (CCSPluginConflict));
	    if (conflict)
	    {
		conflict->refCount = 1;
		conflict->value = strdup (((CCSString *)sl->data)->value);
		conflict->type = ConflictPlugin;
		conflict->plugins =
		    ccsPluginListAppend (conflict->plugins,
			    		 ccsFindPlugin (context, ((CCSString *)sl->data)->value));
		list = ccsPluginConflictListAppend (list, conflict);
	    }
	}

	sl = sl->next;
    }

    return list;
}

CCSPluginConflictList
ccsCanDisablePlugin (CCSContext * context, CCSPlugin * plugin)
{
    CCSPluginConflictList list = NULL;
    CCSPluginConflict *conflict = NULL;
    CCSPluginList pl;
    CCSStringList sl;

    /* look if the plugin to be unloaded is required by another plugin */
    pl = context->plugins;

    for (; pl; pl = pl->next)
    {
	CCSStringList pluginList;

	if (pl->data == plugin)
	    continue;

	if (!ccsPluginIsActive (context, pl->data->name))
	    continue;

	pluginList = pl->data->requiresPlugin;

	while (pluginList)
	{
	    if (strcmp (plugin->name, ((CCSString *)pluginList->data)->value) == 0)
	    {
		if (!conflict)
		{
		    conflict->refCount = 1;
		    conflict = calloc (1, sizeof (CCSPluginConflict));
		    if (conflict)
		    {
			conflict->value = strdup (plugin->name);
			conflict->type = ConflictPluginNeeded;
		    }
		}

		if (conflict)
		    conflict->plugins =
			ccsPluginListAppend (conflict->plugins, pl->data);
		break;
	    }
	    pluginList = pluginList->next;
	}
    }

    if (conflict)
    {
	list = ccsPluginConflictListAppend (list, conflict);
	conflict = NULL;
    }

    /* look if a feature provided is required by another plugin */
    sl = plugin->providesFeature;
    while (sl)
    {
	pl = context->plugins;
	for (; pl; pl = pl->next)
	{
	    CCSStringList pluginList;

	    if (pl->data == plugin)
		continue;

	    if (!ccsPluginIsActive (context, pl->data->name))
		continue;

	    pluginList = pl->data->requiresFeature;

	    while (pluginList)
	    {
		if (strcmp (((CCSString *)sl->data)->value, ((CCSString *)pluginList->data)->value) == 0)
		{
		    if (!conflict)
		    {
			conflict = calloc (1, sizeof (CCSPluginConflict));

			if (conflict)
			{
			    conflict->refCount = 1;
			    conflict->value = strdup (((CCSString *)sl->data)->value);
			    conflict->type = ConflictFeatureNeeded;
			}
		    }
		    if (conflict)
			conflict->plugins =
			    ccsPluginListAppend (conflict->plugins, pl->data);
		}
		pluginList = pluginList->next;
	    }
	    
	}
	if (conflict)
	    list = ccsPluginConflictListAppend (list, conflict);
	conflict = NULL;
	sl = sl->next;
    }

    return list;
}

CCSStringList
ccsGetExistingProfiles (CCSContext * context)
{
    if (!context)
	return NULL;
    
    CONTEXT_PRIV (context);
    
    if (!cPrivate->backend)
	return NULL;

    if (cPrivate->backend->vTable->getExistingProfiles)
	return (*cPrivate->backend->vTable->getExistingProfiles) (context);

    return NULL;
}

void
ccsDeleteProfile (CCSContext * context, char *name)
{
    if (!context)
	return;
    
    CONTEXT_PRIV (context);
    
    if (!cPrivate->backend)
	return;

    /* never ever delete default profile */
    if (!name || !strlen (name))
	return;

    /* if the current profile should be deleted,
       switch to default profile first */
    if (strcmp (cPrivate->profile, name) == 0)
	ccsSetProfile (context, "");

    if (cPrivate->backend->vTable->deleteProfile)
	(*cPrivate->backend->vTable->deleteProfile) (context, name);
}

static void
addBackendInfo (CCSBackendInfoList * bl, char *file)
{
    void *dlhand = NULL;
    char *err = NULL;
    Bool found = FALSE;
    CCSBackendInfo *info;

    dlerror ();

    dlhand = dlopen (file, RTLD_LAZY | RTLD_LOCAL);
    err = dlerror ();
    if (err || !dlhand)
	return;

    BackendGetInfoProc getInfo = dlsym (dlhand, "getBackendInfo");
    if (!getInfo)
    {
	dlclose (dlhand);
	return;
    }

    CCSBackendVTable *vt = getInfo ();
    if (!vt)
    {
	dlclose (dlhand);
	return;
    }

    CCSBackendInfoList l = *bl;
    while (l)
    {
	if (!strcmp (l->data->name, vt->name))
	{
	    found = TRUE;
	    break;
	}

	l = l->next;
    }

    if (found)
    {
	dlclose (dlhand);
	return;
    }

    info = calloc (1, sizeof (CCSBackendInfo));
    if (!info)
    {
	dlclose (dlhand);
	return;
    }

    info->refCount = 1;
    info->name = strdup (vt->name);
    info->shortDesc = (vt->shortDesc) ? strdup (vt->shortDesc) : strdup ("");
    info->longDesc = (vt->longDesc) ? strdup (vt->longDesc) : strdup ("");
    info->integrationSupport = vt->integrationSupport;
    info->profileSupport = vt->profileSupport;

    *bl = ccsBackendInfoListAppend (*bl, info);
    dlclose (dlhand);
}

static int
backendNameFilter (const struct dirent *name)
{
    int length = strlen (name->d_name);

    if (length < 7)
	return 0;

    if (strncmp (name->d_name, "lib", 3) ||
	strncmp (name->d_name + length - 3, ".so", 3))
	return 0;

    return 1;
}

static void
getBackendInfoFromDir (CCSBackendInfoList * bl, char *path)
{

    struct dirent **nameList;
    int nFile, i;

    if (!path)
	return;

    nFile = scandir (path, &nameList, backendNameFilter, NULL);
    if (nFile <= 0)
	return;

    for (i = 0; i < nFile; i++)
    {
	char file[1024];
	sprintf (file, "%s/%s", path, nameList[i]->d_name);
	addBackendInfo (bl, file);
	free (nameList[i]);
    }

    free (nameList);

}

CCSBackendInfoList
ccsGetExistingBackends ()
{
    CCSBackendInfoList rv = NULL;
    char *home = getenv ("HOME");
    char *backenddir;

    if (home && strlen (home))
    {
	asprintf (&backenddir, "%s/.compizconfig/backends", home);
	getBackendInfoFromDir (&rv, backenddir);
	free (backenddir);
    }

    asprintf (&backenddir, "%s/compizconfig/backends", LIBDIR);

    getBackendInfoFromDir (&rv, backenddir);
    free (backenddir);
    return rv;
}

Bool
ccsExportToFile (CCSContext *context,
		 const char *fileName,
		 Bool       skipDefaults)
{
    IniDictionary *exportFile;
    CCSPluginList p;
    CCSSettingList s;
    CCSPlugin *plugin;
    CCSSetting *setting;
    char *keyName;

    exportFile = ccsIniNew ();
    if (!exportFile)
	return FALSE;

    for (p = context->plugins; p; p = p->next)
    {
	plugin = p->data;
	PLUGIN_PRIV (plugin);

	if (!pPrivate->loaded)
	    ccsLoadPluginSettings (plugin);

	for (s = pPrivate->settings; s; s = s->next)
	{
	    setting = s->data;

	    if (skipDefaults && setting->isDefault)
		continue;

	    asprintf (&keyName, "s%d_%s", 
		      context->screenNum, setting->name);

	    switch (setting->type)
	    {
	    case TypeBool:
		ccsIniSetBool (exportFile, plugin->name, keyName,
			       setting->value->value.asBool);
		break;
	    case TypeInt:
		ccsIniSetInt (exportFile, plugin->name, keyName,
			      setting->value->value.asInt);
		break;
	    case TypeFloat:
		ccsIniSetFloat (exportFile, plugin->name, keyName,
				setting->value->value.asFloat);
		break;
	    case TypeString:
		ccsIniSetString (exportFile, plugin->name, keyName,
				 setting->value->value.asString);
		break;
	    case TypeKey:
		ccsIniSetKey (exportFile, plugin->name, keyName,
			      setting->value->value.asKey);
		break;
	    case TypeButton:
		ccsIniSetButton (exportFile, plugin->name, keyName,
				 setting->value->value.asButton);
		break;
	    case TypeEdge:
		ccsIniSetEdge (exportFile, plugin->name, keyName,
			       setting->value->value.asEdge);
		break;
	    case TypeBell:
		ccsIniSetBell (exportFile, plugin->name, keyName,
			       setting->value->value.asBell);
		break;
	    case TypeColor:
		ccsIniSetColor (exportFile, plugin->name, keyName,
				setting->value->value.asColor);
		break;
	    case TypeMatch:
		ccsIniSetString (exportFile, plugin->name, keyName,
				 setting->value->value.asMatch);
		break;
	    case TypeList:
		ccsIniSetList (exportFile, plugin->name, keyName,
			       setting->value->value.asList,
			       setting->info.forList.listType);
		break;
	    default:
		break;
	    }
	    free (keyName);
	}
    }

    ccsIniSave (exportFile, fileName);
    ccsIniClose (exportFile);

    return TRUE;
}

/* + with a value will attempt to append or overwrite that value if there is no -
 * - with a value will attempt to clear any value set to that to the default
 * if there is no +
 * - with a value and + with a value will replace one with the other
 * - without a value and + with a value will ensure that + overwrites the setting
 */

static Bool
ccsProcessSettingPlus (IniDictionary	   *dict,
		       CCSContext 	   *context,
		       CCSSettingsUpgrade  *upgrade,
		       CCSSetting	   *setting)
{
    char         *keyName = NULL;
    char         *sectionName = strdup (setting->parent->name);
    char         *iniValue = NULL;

    asprintf (&keyName, "+s%d_%s", context->screenNum, setting->name);
    if (ccsIniGetString (dict, sectionName, keyName, &iniValue))
    {
	CCSSetting *newSetting = malloc (sizeof (CCSSetting));

	if (!newSetting)
	    return FALSE;

	copySetting (setting, newSetting);

	switch (newSetting->type)
	{
	    case TypeInt:
	    {
		int value;
		ccsIniParseInt (iniValue, &value);

		ccsSetInt (newSetting, value, FALSE);
		break;
	    }
	    case TypeBool:
	    {
		Bool value;
		ccsIniParseBool (iniValue, &value);

		ccsSetBool (newSetting, value, FALSE);
		break;
	    }
	    case TypeFloat:
	    {
		float value;
		ccsIniParseFloat (iniValue, &value);

		ccsSetFloat (newSetting, value, FALSE);
		break;
	    }
	    case TypeString:
	    {
		char *value;
		ccsIniParseString (iniValue, &value);

		ccsSetString (newSetting, value, FALSE);
		free (value);
		break;
	    }
	    case TypeColor:
	    {
		CCSSettingColorValue value;
		ccsIniParseColor (iniValue, &value);

		ccsSetColor (newSetting, value, FALSE);
		break;
	    }
	    case TypeKey:
	    {
		CCSSettingKeyValue value;
		ccsIniParseKey (iniValue, &value);

		ccsSetKey (newSetting, value, FALSE);
		break;
	    }
	    case TypeButton:
	    {
		CCSSettingButtonValue value;
		ccsIniParseButton (iniValue, &value);

		ccsSetButton (newSetting, value, FALSE);
		break;
	    }
	    case TypeEdge:
	    {
		unsigned int	value;
		ccsIniParseEdge (iniValue, &value);

		ccsSetEdge (newSetting, value, FALSE);
		break;
	    }
	    case TypeBell:
	    {
		Bool value;
		ccsIniParseBool (iniValue, &value);

		ccsSetBell (newSetting, value, FALSE);
		break;
	    }
	    case TypeMatch:
	    {
		char  *value;
		ccsIniParseString (iniValue, &value);

		ccsSetMatch (newSetting, value, FALSE);
		free (value);
		break;
	    }
	    case TypeList:
	    {
		CCSSettingValueList value;
		ccsIniParseList (iniValue, &value, newSetting);

		ccsSetList (newSetting, value, FALSE);
		ccsSettingValueListFree (value, TRUE);
		break;
	    }
	    case TypeAction:
	    default:
		/* FIXME: cleanup */
		return FALSE;
	}

        CCSSettingList listIter = upgrade->clearValueSettings;

        while (listIter)
	{
	    CCSSetting *s = (CCSSetting *) listIter->data;

	    if (strcmp (s->name, newSetting->name) == 0)
	    {
		upgrade->replaceToValueSettings = ccsSettingListAppend (upgrade->replaceToValueSettings, (void *) newSetting);
		upgrade->replaceFromValueSettings = ccsSettingListAppend (upgrade->replaceFromValueSettings, (void *) s);
		upgrade->clearValueSettings = ccsSettingListRemove (upgrade->clearValueSettings, (void *) s, FALSE);
		break;
	    }

	    listIter = listIter->next;
	}

	if (!listIter)
	    upgrade->addValueSettings = ccsSettingListAppend (upgrade->addValueSettings, (void *) newSetting);
	
	free (keyName);
	free (sectionName);
	free (iniValue);
	
	return TRUE;
    }
    
    free (keyName);
    free (sectionName);
    
    return FALSE;
}

static Bool
ccsProcessSettingMinus (IniDictionary      *dict,
			CCSContext 	   *context,
			CCSSettingsUpgrade *upgrade,
			CCSSetting	   *setting)
{
    char         *keyName = NULL;
    char         *sectionName = strdup (setting->parent->name);
    char         *iniValue = NULL;

    asprintf (&keyName, "-s%d_%s", context->screenNum, setting->name);
    if (ccsIniGetString (dict, sectionName, keyName, &iniValue))
    {
	CCSSetting *newSetting = malloc (sizeof (CCSSetting));

	if (!newSetting)
	    return FALSE;

	copySetting (setting, newSetting);

	switch (newSetting->type)
	{
	    case TypeInt:
	    {
		int value;
		ccsIniParseInt (iniValue, &value);

		ccsSetInt (newSetting, value, FALSE);
		break;
	    }
	    case TypeBool:
	    {
		Bool value;
		ccsIniParseBool (iniValue, &value);

		ccsSetBool (newSetting, value, FALSE);
		break;
	    }
	    case TypeFloat:
	    {
		float value;
		ccsIniParseFloat (iniValue, &value);

		ccsSetFloat (newSetting, value, FALSE);
		break;
	    }
	    case TypeString:
	    {
		char *value;
		ccsIniParseString (iniValue, &value);

		ccsSetString (newSetting, value, FALSE);
		free (value);
		break;
	    }
	    case TypeColor:
	    {
		CCSSettingColorValue value;
		ccsIniParseColor (iniValue, &value);

		ccsSetColor (newSetting, value, FALSE);
		break;
	    }
	    case TypeKey:
	    {
		CCSSettingKeyValue value;
		ccsIniParseKey (iniValue, &value);

		ccsSetKey (newSetting, value, FALSE);
		break;
	    }
	    case TypeButton:
	    {
		CCSSettingButtonValue value;
		ccsIniParseButton (iniValue, &value);

		ccsSetButton (newSetting, value, FALSE);
		break;
	    }
	    case TypeEdge:
	    {
		unsigned int	value;
		ccsIniParseEdge (iniValue, &value);

		ccsSetEdge (newSetting, value, FALSE);
		break;
	    }
	    case TypeBell:
	    {
		Bool value;
		ccsIniParseBool (iniValue, &value);

		ccsSetBell (newSetting, value, FALSE);
		break;
	    }
	    case TypeMatch:
	    {
		char  *value;
		ccsIniParseString (iniValue, &value);

		ccsSetMatch (newSetting, value, FALSE);
		free (value);
		break;
	    }
	    case TypeList:
	    {
		CCSSettingValueList value;
		ccsIniParseList (iniValue, &value, newSetting);

		ccsSetList (newSetting, value, FALSE);
		ccsSettingValueListFree (value, TRUE);
		break;
	    }
	    case TypeAction:
	    default:
		/* FIXME: cleanup */
		return FALSE;
	}

        CCSSettingList listIter = upgrade->addValueSettings;

        while (listIter)
	{
	    CCSSetting *s = (CCSSetting *) listIter->data;

	    if (strcmp (s->name, newSetting->name) == 0)
	    {
		upgrade->replaceFromValueSettings = ccsSettingListAppend (upgrade->replaceFromValueSettings, (void *) newSetting);
		upgrade->replaceToValueSettings = ccsSettingListAppend (upgrade->replaceToValueSettings, (void *) s);
		upgrade->addValueSettings = ccsSettingListRemove (upgrade->addValueSettings, (void *) s, FALSE);
		break;
	    }

	    listIter = listIter->next;
	}

	if (!listIter)
	    upgrade->clearValueSettings = ccsSettingListAppend (upgrade->clearValueSettings, (void *) newSetting);
	
	free (keyName);
	free (sectionName);
	free (iniValue);
	
	return TRUE;
    }
    
    free (keyName);
    free (sectionName);

    return FALSE;
}

Bool
ccsProcessUpgrade (CCSContext *context,
		   CCSSettingsUpgrade *upgrade)
{
    IniDictionary      *dict = ccsIniOpen (upgrade->file);
    CCSPluginList      pl = context->plugins;
    CCSSettingList     sl;

    ccsSetProfile (context, upgrade->profile);

    while (pl)
    {
	sl = ccsGetPluginSettings ((CCSPlugin *) pl->data);

	while (sl)
	{
	    CCSSetting   *setting = sl->data;

	    ccsProcessSettingMinus (dict, context, upgrade, setting);
	    ccsProcessSettingPlus (dict, context, upgrade, setting);

	    sl = sl->next;
	}

	pl = pl->next;
    }
    
    sl = upgrade->clearValueSettings;
	
    while (sl)
    {
	CCSSetting *tempSetting = (CCSSetting *) sl->data;
	CCSSetting *setting;
	
	setting = ccsFindSetting (tempSetting->parent, tempSetting->name);
	
	if (setting)
	{
	    if (setting->type != TypeList)
	    {
		if (setting->value == tempSetting->value)
		{
		    D (D_FULL, "Resetting %s to default\n", ((CCSSetting *) sl->data)->name);
		    ccsResetToDefault (setting, TRUE);
		}
		else
		{
		    D (D_FULL, "Skipping processing of %s\n", ((CCSSetting *) sl->data)->name);
		}
	    }
	    else
	    {
		unsigned int count = 0;
		/* Try and remove any specified items from the list */
		CCSSettingValueList l = tempSetting->value->value.asList;
		CCSSettingValueList nl = ccsCopyList (setting->value->value.asList, setting);

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

		D (D_FULL, "Removed %i items from %s\n", count, setting->name);
		ccsSetList (setting, nl, TRUE);

	    }
	}

	sl = sl->next;
    }

    sl = upgrade->addValueSettings;
    
    while (sl)
    {
	CCSSetting *tempSetting = (CCSSetting *) sl->data;
	CCSSetting *setting;
	
	setting = ccsFindSetting (tempSetting->parent, tempSetting->name);
	
	if (setting)
	{
	    D (D_FULL, "Overriding value %s\n", ((CCSSetting *) sl->data)->name);
	    if (setting->type != TypeList)
		ccsSetValue (setting, tempSetting->value, TRUE);
	    else
	    {
		unsigned int count = 0;
		/* Try and apppend any new items to the list */
		CCSSettingValueList l = tempSetting->value->value.asList;
		CCSSettingValueList nl = ccsCopyList (setting->value->value.asList, setting);
		
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

		D (D_FULL, "Appending %i items to %s\n", count, setting->name);
		ccsSetList (setting, nl, TRUE);
	    }
	}
	else
	{
	    D (D_FULL, "Value %s not found!\n", ((CCSSetting *) sl->data)->name);
	}

	sl = sl->next;
    }

    sl = upgrade->replaceFromValueSettings;
    
    while (sl)
    {
	CCSSetting *tempSetting = (CCSSetting *) sl->data;
	CCSSetting *setting;
	
	setting = ccsFindSetting (tempSetting->parent, tempSetting->name);
	
	if (setting)
	{
	    if (setting->value == tempSetting->value)
	    {
		CCSSettingList rl = upgrade->replaceToValueSettings;
		
		while (rl)
		{
		    CCSSetting *rsetting = (CCSSetting *) rl->data;
		    
		    if (strcmp (rsetting->name, setting->name) == 0)
		    {
			D (D_FULL, "Matched and replaced %s\n", setting->name);
			ccsSetValue (setting, rsetting->value, TRUE);
			break;
		    }
		    
		    rl = rl->next;
		}
	    }
	    else
	    {
		D (D_FULL, "Skipping processing of %s\n", ((CCSSetting *) sl->data)->name);
	    }
	}

	sl = sl->next;
    }
    
    upgrade->clearValueSettings = ccsSettingListFree (upgrade->clearValueSettings, TRUE);
    upgrade->addValueSettings = ccsSettingListFree (upgrade->addValueSettings, TRUE);
    upgrade->replaceFromValueSettings = ccsSettingListFree (upgrade->replaceFromValueSettings, TRUE);
    upgrade->replaceToValueSettings = ccsSettingListFree (upgrade->replaceToValueSettings, TRUE);

    ccsIniClose (dict);

    return TRUE;
}

static int
upgradeNameFilter (const struct dirent *name)
{
    int length = strlen (name->d_name);
    char *uname, *tok;

    if (length < 7)
	return 0;

    uname = tok = strdup (name->d_name);

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

/*
 * Process a filename into the properties
 * for a settings upgrade
 * eg
 * 
 * org.freedesktop.compiz.Default.01.upgrade
 * 
 * gives us:
 * domain: org.freedesktop.compiz
 * number: 1
 * profile: Default
 * 
 */
CCSSettingsUpgrade *
ccsSettingsUpgradeNew (char *path, char *name)
{
    CCSSettingsUpgrade *upgrade = calloc (1, sizeof (CCSSettingsUpgrade));
    int length = strlen (name);
    char *uname, *tok;
    unsigned int fnlen = strlen (path) + strlen (name) + 1;

    upgrade->file = calloc (fnlen + 1, sizeof (char));
    sprintf (upgrade->file, "%s/%s", path, name);

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
	    upgrade->domain = strndup (uname, length - (strlen (tok) + 1));
	    upgrade->num = num;

	    /* profile.n.upgrade */
	    upgrade->profile = strndup (tok, strlen (tok) - (strlen (nexttok) + 1));
	    free (bit);
	    break;
	}
	else if (errno)
	    perror ("strtol");
	
	tok = nexttok;
	
	free (bit);
    }
    
    free (uname);
    
    return upgrade;
}

Bool
ccsCheckForSettingsUpgrade (CCSContext *context)
{
    struct dirent 	   **nameList;
    int 	  	   nFile, i;
    char	  	   *path = DATADIR "/compizconfig/upgrades/";
    char		   *dupath = NULL;
    FILE		   *completedUpgrades;
    char		   *cuBuffer;
    unsigned int	   cuSize;
    size_t		   cuReadSize;
    char		   *home = getenv ("HOME");

    if (!home)
	return FALSE;

    asprintf (&dupath, "%s/.config/compiz-1/compizconfig/done_upgrades", home);

    completedUpgrades = fopen (dupath, "a+");

    if (!path)
	return FALSE;

    nFile = scandir (path, &nameList, upgradeNameFilter, alphasort);
    if (nFile <= 0)
	return FALSE;

    if (!completedUpgrades)
    {
	fprintf (stderr, "[WARNING] Error opening done_upgrades\n");
	return FALSE;
    }

    fseek (completedUpgrades, 0, SEEK_END);
    cuSize = ftell (completedUpgrades);
    rewind (completedUpgrades);

    cuBuffer = calloc (cuSize + 1, sizeof (char));
    cuReadSize = fread (cuBuffer, 1, cuSize, completedUpgrades);

    if (cuReadSize != cuSize)
	D (D_FULL, "[WARNING] Couldn't read completed upgrades file!\n");

    cuBuffer[cuSize] = '\0';

    for (i = 0; i < nFile; i++)
    {
	char		   *matched = strstr (cuBuffer, nameList[i]->d_name);
	CCSSettingsUpgrade *upgrade;
	
	if (matched)
	{
	    D (D_FULL, "Skipping upgrade %s\n", nameList[i]->d_name);
	    continue;
	}

	upgrade = ccsSettingsUpgradeNew (path, nameList[i]->d_name);
	
	D (D_FULL, "Processing upgrade %s\nprofile: %s\nnumber: %i\ndomain: %s\n", nameList[i]->d_name, upgrade->profile, upgrade->num, upgrade->domain);

	ccsProcessUpgrade (context, upgrade);
	ccsWriteChangedSettings (context);
	ccsWriteAutoSortedPluginList (context);
	D (D_FULL, "Completed upgrade %s\n", nameList[i]->d_name);
	fprintf (completedUpgrades, "%s\n", nameList[i]->d_name);
	free (upgrade->profile);
	free (upgrade->domain);
	free (upgrade->file);
	free (upgrade);
	free (nameList[i]);
    }

    free (dupath);
    fclose (completedUpgrades);
    free (cuBuffer);
    free (nameList);
    
    return TRUE;
}

Bool
ccsImportFromFile (CCSContext *context,
		   const char *fileName,
		   Bool       overwriteNonDefault)
{
    IniDictionary *importFile;
    CCSPluginList p;
    CCSSettingList s;
    CCSPlugin *plugin;
    CCSSetting *setting;
    char *keyName;
    FILE *fp;

    /* check if the file exists first */
    fp = fopen (fileName, "r");
    if (!fp)
	return FALSE;
    fclose (fp);

    importFile = iniparser_new ((char *) fileName);
    if (!importFile)
	return FALSE;

    for (p = context->plugins; p; p = p->next)
    {
	plugin = p->data;
	PLUGIN_PRIV (plugin);

	if (!pPrivate->loaded)
	    ccsLoadPluginSettings (plugin);

	for (s = pPrivate->settings; s; s = s->next)
	{
	    setting = s->data;
	    if (!setting->isDefault && !overwriteNonDefault)
		continue;

	    asprintf (&keyName, "s%d_%s", 
		      context->screenNum, setting->name);

	    switch (setting->type)
	    {
	    case TypeBool:
		{
		    Bool value;

		    if (ccsIniGetBool (importFile, plugin->name,
				       keyName, &value))
		    {
			ccsSetBool (setting, value, TRUE);
		    }
		}
		break;
	    case TypeInt:
		{
		    int value;

		    if (ccsIniGetInt (importFile, plugin->name,
				      keyName, &value))
			ccsSetInt (setting, value, TRUE);
		}
		break;
	    case TypeFloat:
		{
		    float value;

		    if (ccsIniGetFloat (importFile, plugin->name,
					keyName, &value))
			ccsSetFloat (setting, value, TRUE);
		}
		break;
	    case TypeString:
		{
		    char *value;

		    if (ccsIniGetString (importFile, plugin->name,
					 keyName, &value))
		    {
		    	ccsSetString (setting, value, TRUE);
		    	free (value);
		    }
		}
		break;
	    case TypeKey:
		{
		    CCSSettingKeyValue value;

		    if (ccsIniGetKey (importFile, plugin->name,
				      keyName, &value))
			ccsSetKey (setting, value, TRUE);
		}
		break;
	    case TypeButton:
		{
		    CCSSettingButtonValue value;

		    if (ccsIniGetButton (importFile, plugin->name,
					 keyName, &value))
			ccsSetButton (setting, value, TRUE);
		}
		break;
	    case TypeEdge:
		{
		    unsigned int value;

		    if (ccsIniGetEdge (importFile, plugin->name,
				       keyName, &value))
			ccsSetEdge (setting, value, TRUE);
		}
		break;
	    case TypeBell:
		{
		    Bool value;

		    if (ccsIniGetBell (importFile, plugin->name,
				       keyName, &value))
			ccsSetBell (setting, value, TRUE);
		}
		break;
	    case TypeColor:
		{
		    CCSSettingColorValue value;

		    if (ccsIniGetColor (importFile, plugin->name,
					keyName, &value))
			ccsSetColor (setting, value, TRUE);
		}
		break;
	    case TypeMatch:
		{
		    char *value;
		    if (ccsIniGetString (importFile, plugin->name,
					 keyName, &value))
		    {
		    	ccsSetMatch (setting, value, TRUE);
		    	free (value);
		    }
		}
		break;
	    case TypeList:
		{
		    CCSSettingValueList value;
		    if (ccsIniGetList (importFile, plugin->name,
				       keyName, &value, setting))
		    {
			ccsSetList (setting, value, TRUE);
			ccsSettingValueListFree (value, TRUE);
		    }
		}
		break;
	    default:
		break;
	    }

	    free (keyName);
	}
    }

    ccsIniClose (importFile);

    return TRUE;
}

CCSSettingList ccsGetPluginSettings (CCSPlugin *plugin)

{
    PLUGIN_PRIV (plugin);

    if (!pPrivate->loaded)
	ccsLoadPluginSettings (plugin);

    return pPrivate->settings;
}

CCSGroupList ccsGetPluginGroups (CCSPlugin *plugin)
{
    PLUGIN_PRIV (plugin);

    if (!pPrivate->loaded)
	ccsLoadPluginSettings (plugin);

    return pPrivate->groups;
}

Bool ccsSettingIsIntegrated (CCSSetting *setting)
{
    if (!setting)
	return FALSE;

    CONTEXT_PRIV (setting->parent->context);

    if (!cPrivate->backend)
	return FALSE;

    if (cPrivate->backend->vTable->getSettingIsIntegrated)
	return (*cPrivate->backend->vTable->getSettingIsIntegrated) (setting);

    return FALSE;
}

Bool ccsSettingIsReadOnly (CCSSetting *setting)
{
    if (!setting)
	return FALSE;

    CONTEXT_PRIV (setting->parent->context);

    if (!cPrivate->backend)
	return FALSE;

    if (cPrivate->backend->vTable->getSettingIsReadOnly)
	return (*cPrivate->backend->vTable->getSettingIsReadOnly) (setting);

    return FALSE;
}

CCSStrExtensionList ccsGetPluginStrExtensions (CCSPlugin *plugin)
{
    PLUGIN_PRIV (plugin);

    if (!pPrivate->loaded)
	ccsLoadPluginSettings (plugin);

    return pPrivate->stringExtensions;
}

