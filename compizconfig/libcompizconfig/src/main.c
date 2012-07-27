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

static void * wrapRealloc (void *o, void *a , size_t b)
{
    return realloc (a, b);
}

static void * wrapMalloc (void *o, size_t a)
{
    return malloc (a);
}

static void * wrapCalloc (void *o, size_t a, size_t b)
{
    return calloc (a, b);
}

static void wrapFree (void *o, void *a)
{
    free (a);
}

CCSObjectAllocationInterface ccsDefaultObjectAllocator =
{
    wrapRealloc,
    wrapMalloc,
    wrapCalloc,
    wrapFree,
    NULL
};

/* CCSObject stuff */
Bool
ccsObjectInit_(CCSObject *object, CCSObjectAllocationInterface *object_allocation)
{
    object->priv = NULL;
    object->n_interfaces = 0;
    object->n_allocated_interfaces = 0;
    object->interfaces = NULL;
    object->interface_types = NULL;
    object->object_allocation = object_allocation;
    object->refcnt = 0;

    return TRUE;
}

Bool
ccsObjectAddInterface_(CCSObject *object, const CCSInterface *interface, int interface_type)
{
    object->n_interfaces++;

    if (object->n_allocated_interfaces < object->n_interfaces)
    {
	unsigned int old_allocated_interfaces = object->n_allocated_interfaces;
	object->n_allocated_interfaces = object->n_interfaces;
	CCSInterface **ifaces = (*object->object_allocation->realloc_) (object->object_allocation->allocator, object->interfaces, object->n_allocated_interfaces * sizeof (CCSInterface *));
	int          *iface_types = (*object->object_allocation->realloc_) (object->object_allocation->allocator, object->interface_types, object->n_allocated_interfaces * sizeof (int));

	if (!ifaces || !iface_types)
	{
	    if (ifaces)
		(*object->object_allocation->free_) (object->object_allocation->allocator, ifaces);

	    if (iface_types)
		(*object->object_allocation->free_) (object->object_allocation->allocator, iface_types);

	    object->n_interfaces--;
	    object->n_allocated_interfaces = old_allocated_interfaces;
	    return FALSE;
	}
	else
	{
	    object->interfaces = (const CCSInterface **) ifaces;
	    object->interface_types = iface_types;
	}
    }

    object->interfaces[object->n_interfaces - 1] = interface;
    object->interface_types[object->n_interfaces - 1] = interface_type;

    return TRUE;
}

Bool
ccsObjectRemoveInterface_(CCSObject *object, int interface_type)
{
    unsigned int i = 0;

    if (!object->n_interfaces)
	return FALSE;

    const CCSInterface **o = object->interfaces;
    int        *type = object->interface_types;

    for (; i < object->n_interfaces; i++, o++, type++)
    {
	if (object->interface_types[i] == interface_type)
	    break;
    }

    if (i >= object->n_interfaces)
	return FALSE;

    /* Now clear this section and move everything back */
    object->interfaces[i] = NULL;

    i++;

    const CCSInterface **oLast = o;
    int *typeLast = type;

    o++;
    type++;

    memmove ((void *) oLast, (void *)o, (object->n_interfaces - i) * sizeof (CCSInterface *));
    memmove ((void *) typeLast, (void *) type, (object->n_interfaces - i) * sizeof (int));

    object->n_interfaces--;

    if (!object->n_interfaces)
    {
	free (object->interfaces);
	free (object->interface_types);
	object->interfaces = NULL;
	object->interface_types = NULL;
	object->n_allocated_interfaces = 0;
    }

    return TRUE;
}

const CCSInterface *
ccsObjectGetInterface_(CCSObject *object, int interface_type)
{
    unsigned int i = 0;

    for (; i < object->n_interfaces; i++)
    {
	if (object->interface_types[i] == interface_type)
	    return object->interfaces[i];
    }

    return NULL;
}

CCSPrivate *
ccsObjectGetPrivate_(CCSObject *object)
{
    return object->priv;
}

void
ccsObjectSetPrivate_(CCSObject *object, CCSPrivate *priv)
{
    object->priv = priv;
}

void
ccsObjectFinalize_(CCSObject *object)
{
    if (object->priv)
    {
	(*object->object_allocation->free_) (object->object_allocation->allocator, object->priv);
	object->priv = NULL;
    }

    if (object->interfaces)
    {
	(*object->object_allocation->free_) (object->object_allocation->allocator, object->interfaces);
	object->interfaces = NULL;
    }

    if (object->interface_types)
    {
	(*object->object_allocation->free_) (object->object_allocation->allocator, object->interface_types);
	object->interface_types = NULL;
    }

    object->n_interfaces = 0;
}

unsigned int
ccsAllocateType ()
{
    static unsigned int start = 0;

    start++;

    return start;
}

INTERFACE_TYPE (CCSContextInterface)
INTERFACE_TYPE (CCSPluginInterface)
INTERFACE_TYPE (CCSSettingInterface)

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
ccsEmptyContextNew (unsigned int screenNum, const CCSInterfaceTable *object_interfaces)
{
    CCSContext *context;

    context = calloc (1, sizeof (CCSContext));

    if (!context)
	return NULL;

    ccsObjectInit (context, &ccsDefaultObjectAllocator);

    CCSContextPrivate *ccsPrivate = calloc (1, sizeof (CCSContextPrivate));
    if (!ccsPrivate)
    {
	free (context);
	return NULL;
    }

    ccsObjectSetPrivate (context, (CCSPrivate *) ccsPrivate);

    CONTEXT_PRIV (context);

    cPrivate->object_interfaces = object_interfaces;
    cPrivate->screenNum = screenNum;

    ccsObjectAddInterface (context, (CCSInterface *) object_interfaces->contextInterface, GET_INTERFACE_TYPE (CCSContextInterface));

    initGeneralOptions (context);
    cPrivate->configWatchId = ccsAddConfigWatch (context, configChangeNotify);

    if (cPrivate->backend)
	ccsInfo ("Backend     : %s", cPrivate->backend->vTable->name);
	ccsInfo ("Integration : %s", cPrivate->deIntegration ? "true" : "false");
	ccsInfo ("Profile     : %s",
	    (cPrivate->profile && strlen (cPrivate->profile)) ?
	    cPrivate->profile : "default");

    return context;
}

static void *
ccsContextGetPrivatePtrDefault (CCSContext *context)
{
    CONTEXT_PRIV (context);

    return cPrivate->privatePtr;
}

static void
ccsContextSetPrivatePtrDefault (CCSContext *context, void *ptr)
{
    CONTEXT_PRIV (context);

    cPrivate->privatePtr = ptr;
}

static CCSPluginList
ccsContextGetPluginsDefault (CCSContext *context)
{
    CONTEXT_PRIV (context);

    return cPrivate->plugins;
}

static CCSPluginCategory *
ccsContextGetCategoriesDefault (CCSContext *context)
{
    CONTEXT_PRIV (context);

    return cPrivate->categories;
}

static CCSSettingList
ccsContextGetChangedSettingsDefault (CCSContext *context)
{
    CONTEXT_PRIV (context);

    return cPrivate->changedSettings;
}

static unsigned int
ccsContextGetScreenNumDefault (CCSContext *context)
{
    CONTEXT_PRIV (context);

    return cPrivate->screenNum;
}

static Bool
ccsContextAddChangedSettingDefault (CCSContext *context, CCSSetting *setting)
{
    CONTEXT_PRIV (context);

    cPrivate->changedSettings = ccsSettingListAppend (cPrivate->changedSettings, setting);

    return TRUE;
}

static Bool
ccsContextClearChangedSettingsDefault (CCSContext *context)
{
    CONTEXT_PRIV (context);

    cPrivate->changedSettings = ccsSettingListFree (cPrivate->changedSettings, FALSE);

    return TRUE;
}

static CCSSettingList
ccsContextStealChangedSettingsDefault (CCSContext *context)
{
    CONTEXT_PRIV (context);

    CCSSettingList l = cPrivate->changedSettings;

    cPrivate->changedSettings = NULL;
    return l;
}

CCSPluginList
ccsContextGetPlugins (CCSContext *context)
{
    return (*(GET_INTERFACE (CCSContextInterface, context))->contextGetPlugins) (context);
}

CCSPluginCategory *
ccsContextGetCategories (CCSContext *context)
{
    return (*(GET_INTERFACE (CCSContextInterface, context))->contextGetCategories) (context);
}

CCSSettingList
ccsContextGetChangedSettings (CCSContext *context)
{
    return (*(GET_INTERFACE (CCSContextInterface, context))->contextGetChangedSettings) (context);
}

unsigned int
ccsContextGetScreenNum (CCSContext *context)
{
    return (*(GET_INTERFACE (CCSContextInterface, context))->contextGetScreenNum) (context);
}

Bool
ccsContextAddChangedSetting (CCSContext *context, CCSSetting *setting)
{
    return (*(GET_INTERFACE (CCSContextInterface, context))->contextAddChangedSetting) (context, setting);
}

Bool
ccsContextClearChangedSettings (CCSContext *context)
{
    return (*(GET_INTERFACE (CCSContextInterface, context))->contextClearChangedSettings) (context);
}

CCSSettingList
ccsContextStealChangedSettings (CCSContext *context)
{
    return (*(GET_INTERFACE (CCSContextInterface, context))->contextStealChangedSettings) (context);
}

void *
ccsContextGetPrivatePtr (CCSContext *context)
{
    return (*(GET_INTERFACE (CCSContextInterface, context))->contextGetPrivatePtr) (context);
}

void
ccsContextSetPrivatePtr (CCSContext *context, void *ptr)
{
    (*(GET_INTERFACE (CCSContextInterface, context))->contextSetPrivatePtr) (context, ptr);
}

void *
ccsContextGetPluginsBindable (CCSContext *context)
{
    return (void *) ccsContextGetPlugins (context);
}

void *
ccsContextStealChangedSettingsBindable (CCSContext *context)
{
    return (void *) ccsContextStealChangedSettings (context);
}

void *
ccsContextGetChangedSettingsBindable (CCSContext *context)
{
    return (void *) ccsContextGetChangedSettings (context);
}


static void
ccsSetActivePluginList (CCSContext * context, CCSStringList list)
{
    CCSPluginList l;
    CCSPlugin     *plugin;

    CONTEXT_PRIV (context);

    for (l = cPrivate->plugins; l; l = l->next)
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
ccsContextNew (unsigned int screenNum, const CCSInterfaceTable *iface)
{
    CCSPlugin  *p;
    CCSContext *context = ccsEmptyContextNew (screenNum, iface);
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
ccsFindPluginDefault (CCSContext * context, const char *name)
{
    if (!name)
	name = "";

    CONTEXT_PRIV (context);

    CCSPluginList l = cPrivate->plugins;
    while (l)
    {
	if (!strcmp (ccsPluginGetName (l->data), name))
	    return l->data;

	l = l->next;
    }

    return NULL;
}

CCSPlugin *
ccsFindPlugin (CCSContext *context, const char *name)
{
    return (*(GET_INTERFACE (CCSContextInterface, context))->contextFindPlugin) (context, name);
}

CCSSetting *
ccsFindSettingDefault (CCSPlugin * plugin, const char *name)
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
	if (!strcmp (ccsSettingGetName (l->data), name))
	    return l->data;

	l = l->next;
    }

    return NULL;
}

CCSSetting *
ccsFindSetting (CCSPlugin *plugin, const char *name)
{
    if (!plugin)
	return NULL;

    return (*(GET_INTERFACE (CCSPluginInterface, plugin))->pluginFindSetting) (plugin, name);
}

Bool
ccsPluginIsActiveDefault (CCSContext * context, char *name)
{
    CCSPlugin *plugin;

    plugin = ccsFindPlugin (context, name);
    if (!plugin)
	return FALSE;

    PLUGIN_PRIV (plugin);

    return pPrivate->active;
}

Bool
ccsPluginIsActive (CCSContext *context, char *name)
{
    return (*(GET_INTERFACE (CCSContextInterface, context))->contextPluginIsActive) (context, name);
}


static void
subGroupAdd (CCSSetting * setting, CCSGroup * group)
{
    CCSSubGroupList l = group->subGroups;
    CCSSubGroup     *subGroup;

    while (l)
    {
	if (!strcmp (l->data->name, ccsSettingGetSubGroup (setting)))
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
	subGroup->name = strdup (ccsSettingGetSubGroup (setting));
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
	if (!strcmp (l->data->name, ccsSettingGetGroup (setting)))
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
	group->name = strdup (ccsSettingGetGroup (setting));
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

    if (cPrivate->changedSettings)
	cPrivate->changedSettings = ccsSettingListFree (cPrivate->changedSettings, FALSE);

    ccsPluginListFree (cPrivate->plugins, TRUE);

    ccsObjectFinalize (c);
    free (c);
}

void
ccsFreePlugin (CCSPlugin * p)
{
    if (!p)
	return;

    PLUGIN_PRIV (p);

    free (pPrivate->name);
    free (pPrivate->shortDesc);
    free (pPrivate->longDesc);
    free (pPrivate->hints);
    free (pPrivate->category);

    ccsStringListFree (pPrivate->loadAfter, TRUE);
    ccsStringListFree (pPrivate->loadBefore, TRUE);
    ccsStringListFree (pPrivate->requiresPlugin, TRUE);
    ccsStringListFree (pPrivate->conflictPlugin, TRUE);
    ccsStringListFree (pPrivate->conflictFeature, TRUE);
    ccsStringListFree (pPrivate->providesFeature, TRUE);
    ccsStringListFree (pPrivate->requiresFeature, TRUE);

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

    ccsObjectFinalize (p);
    free (p);
}

static void
ccsFreeSettingDefault (CCSSetting *s)
{
    SETTING_PRIV (s);

    free (sPrivate->name);
    free (sPrivate->shortDesc);
    free (sPrivate->longDesc);
    free (sPrivate->group);
    free (sPrivate->subGroup);
    free (sPrivate->hints);

    switch (sPrivate->type)
    {
    case TypeInt:
	ccsIntDescListFree (sPrivate->info.forInt.desc, TRUE);
	break;
    case TypeString:
	ccsStrRestrictionListFree (sPrivate->info.forString.restriction, TRUE);
	break;
    case TypeList:
	if (sPrivate->info.forList.listType == TypeInt)
	    ccsIntDescListFree (sPrivate->info.forList.listInfo->
				forInt.desc, TRUE);
	free (sPrivate->info.forList.listInfo);
	//ccsSettingValueListFree (sPrivate->value->value.asList, TRUE);
	break;
    default:
	break;
    }

    if (&sPrivate->defaultValue != sPrivate->value)
    {
	ccsFreeSettingValue (sPrivate->value);
    }

    ccsFreeSettingValue (&sPrivate->defaultValue);

    ccsObjectFinalize (s);
    free (s);
}

void
ccsFreeSetting (CCSSetting * s)
{
    if (!s)
	return;

    (*(GET_INTERFACE (CCSSettingInterface, s))->settingDestructor) (s);
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

    CCSSettingType type = ccsSettingGetType (v->parent);

    if (v->isListChild)
	type = ccsSettingGetInfo (v->parent)->forList.listType;

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

    if (v != ccsSettingGetDefaultValue (v->parent))
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


CCSREF (String, CCSString)
CCSREF (Group, CCSGroup)
CCSREF (SubGroup, CCSSubGroup)
CCSREF (SettingValue, CCSSettingValue)
CCSREF (PluginConflict, CCSPluginConflict)
CCSREF (BackendInfo, CCSBackendInfo)
CCSREF (IntDesc, CCSIntDesc)
CCSREF (StrRestriction, CCSStrRestriction)
CCSREF (StrExtension, CCSStrExtension)

#define CCSREF_OBJ(type,dtype) \
    void ccs##type##Ref (dtype *d) \
    { \
	ccsObjectRef (d); \
    } \
    \
    void ccs##type##Unref (dtype *d) \
    { \
	ccsObjectUnref (d, ccsFree##type); \
    } \

CCSREF_OBJ (Plugin, CCSPlugin)
CCSREF_OBJ (Setting, CCSSetting)

static void *
openBackend (char *backend)
{
    char *home = getenv ("HOME");
    char *override_backend = getenv ("LIBCOMPIZCONFIG_BACKEND_PATH");
    void *dlhand = NULL;
    char *dlname = NULL;
    char *err = NULL;

    if (override_backend && strlen (override_backend))
    {
	if (asprintf (&dlname, "%s/lib%s.so",
		      override_backend, backend) == -1)
	    dlname = NULL;

	dlerror ();
	dlhand = dlopen (dlname, RTLD_NOW | RTLD_NODELETE | RTLD_LOCAL);
	err = dlerror ();
    }

    if (!dlhand && home && strlen (home))
    {
	if (dlname)
	    free (dlname);

	if (asprintf (&dlname, "%s/.compizconfig/backends/lib%s.so",
		      home, backend) == -1)
	    dlname = NULL;

	dlerror ();
	dlhand = dlopen (dlname, RTLD_NOW | RTLD_NODELETE | RTLD_LOCAL);
	err = dlerror ();
    }

    if (!dlhand)
    {
	if (dlname)
	    free (dlname);

	if (asprintf (&dlname, "%s/compizconfig/backends/lib%s.so",
		      LIBDIR, backend) == -1)
	    dlname = NULL;
	dlhand = dlopen (dlname, RTLD_NOW | RTLD_NODELETE | RTLD_LOCAL);
	err = dlerror ();
    }

    free (dlname);

    if (err)
    {
	ccsError ("dlopen: %s", err);
    }

    return dlhand;
}

Bool
ccsSetBackendDefault (CCSContext * context, char *name)
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

Bool
ccsSetBackend (CCSContext *context, char *name)
{
    return (*(GET_INTERFACE (CCSContextInterface, context))->contextSetBackend) (context, name);
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
    CCSSettingType type = ccsSettingGetType (from->parent);

    if (from->isListChild)
	type = ccsSettingGetInfo (from->parent)->forList.listType;

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

/* TODO: CCSSetting is not meant to be copyable ... remove */
static void
copySetting (CCSSetting *from, CCSSetting *to)
{
    memcpy (to, from, sizeof (CCSSetting));

    ccsObjectInit (from, &ccsDefaultObjectAllocator);

    /* Allocate a new private ptr for the new setting */
    CCSSettingPrivate *ccsPrivate = calloc (1, sizeof (CCSSettingPrivate));

    ccsObjectSetPrivate (to, (CCSPrivate *) ccsPrivate);

    CCSSettingPrivate *fromPrivate = (CCSSettingPrivate *) ccsObjectGetPrivate (from);
    CCSSettingPrivate *toPrivate = (CCSSettingPrivate *) ccsObjectGetPrivate (to);

    if (fromPrivate->name)
	toPrivate->name = strdup (fromPrivate->name);
    if (fromPrivate->shortDesc)
	toPrivate->shortDesc = strdup (fromPrivate->shortDesc);
    if (fromPrivate->longDesc)
	toPrivate->longDesc = strdup (fromPrivate->longDesc);
    if (fromPrivate->group)
	toPrivate->group = strdup (fromPrivate->group);
    if (fromPrivate->subGroup)
	toPrivate->subGroup = strdup (fromPrivate->subGroup);
    if (fromPrivate->hints)
	toPrivate->hints = strdup (fromPrivate->hints);
    if (fromPrivate->value)
    {
	toPrivate->value = malloc (sizeof (CCSSettingValue));
	
	if (!fromPrivate->value)
	    return;

	copyValue (fromPrivate->value, toPrivate->value);

	toPrivate->value->refCount = 1;
	toPrivate->value->parent = to;
    }

    copyValue (&fromPrivate->defaultValue, &toPrivate->defaultValue);
    copyInfo (&fromPrivate->info, &toPrivate->info, fromPrivate->type);

    toPrivate->defaultValue.parent = to;
    toPrivate->privatePtr = NULL;
    
    (to)->object.refcnt = 1;
}

static void
copyFromDefault (CCSSetting * setting)
{
    CCSSettingValue *value;

    SETTING_PRIV (setting);

    if (sPrivate->value != &sPrivate->defaultValue)
	ccsFreeSettingValue (sPrivate->value);

    value = calloc (1, sizeof (CCSSettingValue));
    if (!value)
    {
	sPrivate->value = &sPrivate->defaultValue;
	sPrivate->isDefault = TRUE;
	return;
    }
    
    value->refCount = 1;

    copyValue (&sPrivate->defaultValue, value);
    sPrivate->value = value;
    sPrivate->isDefault = FALSE;
}

void
ccsSettingResetToDefaultDefault (CCSSetting * setting, Bool processChanged)
{
    SETTING_PRIV (setting)

    if (sPrivate->value != &sPrivate->defaultValue)
    {
	ccsFreeSettingValue (sPrivate->value);

	if (processChanged)
	    ccsContextAddChangedSetting (ccsPluginGetContext (sPrivate->parent), setting);
    }

    sPrivate->value = &sPrivate->defaultValue;
    sPrivate->isDefault = TRUE;
}

Bool
ccsCheckValueEq (CCSSettingValue *rhs, CCSSettingValue *lhs)
{
    CCSSettingType type;

    if (ccsSettingGetType (rhs->parent) != ccsSettingGetType (lhs->parent))
    {
	ccsWarning ("Attempted to check equality between mismatched types!");
	return FALSE;
    }

    if (rhs->isListChild)
	type = ccsSettingGetInfo (rhs->parent)->forList.listType;
    else
	type = ccsSettingGetType (rhs->parent);
    
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
	    ccsWarning ("Actions are not comparable!");
	    return FALSE;
	case TypeList:
	{
	    return ccsCompareLists (lhs->value.asList, rhs->value.asList,
				    ccsSettingGetInfo (lhs->parent)->forList);
	
	}
	default:
	    break;
    }
    
    ccsWarning ("Failed to process type %i", ccsSettingGetType (lhs->parent));
    return FALSE;
}

/* FIXME: That's a lot of code for the sake of type switching ...
 * maybe we need to switch to C++ here and use templates ... */

Bool
ccsSettingSetIntDefault (CCSSetting * setting, int data, Bool processChanged)
{
    SETTING_PRIV (setting)

    if (sPrivate->type != TypeInt)
	return FALSE;

    if (sPrivate->isDefault && (sPrivate->defaultValue.value.asInt == data))
	return TRUE;

    if (!sPrivate->isDefault && (sPrivate->defaultValue.value.asInt == data))
    {
	ccsResetToDefault (setting, processChanged);
	return TRUE;
    }

    if (sPrivate->value->value.asInt == data)
	return TRUE;

    if ((data < sPrivate->info.forInt.min) ||
	 (data > sPrivate->info.forInt.max))
	return FALSE;

    if (sPrivate->isDefault)
	copyFromDefault (setting);

    sPrivate->value->value.asInt = data;

    if (processChanged)
	ccsContextAddChangedSetting (ccsPluginGetContext (sPrivate->parent), setting);

    return TRUE;
}

Bool
ccsSettingSetFloatDefault (CCSSetting * setting, float data, Bool processChanged)
{
    SETTING_PRIV (setting);

    if (sPrivate->type != TypeFloat)
	return FALSE;

    if (sPrivate->isDefault && (sPrivate->defaultValue.value.asFloat == data))
	return TRUE;

    if (!sPrivate->isDefault && (sPrivate->defaultValue.value.asFloat == data))
    {
	ccsResetToDefault (setting, processChanged);
	return TRUE;
    }

    /* allow the values to differ a tiny bit because of
       possible rounding / precision issues */
    if (fabs (sPrivate->value->value.asFloat - data) < 1e-5)
	return TRUE;

    if ((data < sPrivate->info.forFloat.min) ||
	 (data > sPrivate->info.forFloat.max))
	return FALSE;

    if (sPrivate->isDefault)
	copyFromDefault (setting);

    sPrivate->value->value.asFloat = data;

    if (processChanged)
	ccsContextAddChangedSetting (ccsPluginGetContext (sPrivate->parent), setting);

    return TRUE;
}

Bool
ccsSettingSetBoolDefault (CCSSetting * setting, Bool data, Bool processChanged)
{
    SETTING_PRIV (setting)

    if (sPrivate->type != TypeBool)
	return FALSE;

    if (sPrivate->isDefault
	&& ((sPrivate->defaultValue.value.asBool && data)
	     || (!sPrivate->defaultValue.value.asBool && !data)))
	return TRUE;

    if (!sPrivate->isDefault
	&& ((sPrivate->defaultValue.value.asBool && data)
	     || (!sPrivate->defaultValue.value.asBool && !data)))
    {
	ccsResetToDefault (setting, processChanged);
	return TRUE;
    }

    if ((sPrivate->value->value.asBool && data)
	 || (!sPrivate->value->value.asBool && !data))
	return TRUE;

    if (sPrivate->isDefault)
	copyFromDefault (setting);

    sPrivate->value->value.asBool = data;

    if (processChanged)
	ccsContextAddChangedSetting (ccsPluginGetContext (sPrivate->parent), setting);

    return TRUE;
}

Bool
ccsSettingSetStringDefault (CCSSetting * setting, const char *data, Bool processChanged)
{
    SETTING_PRIV (setting);

    if (sPrivate->type != TypeString)
	return FALSE;

    if (!data)
	return FALSE;

    Bool isDefault = strcmp (sPrivate->defaultValue.value.asString, data) == 0;

    if (sPrivate->isDefault && isDefault)
	return TRUE;

    if (!sPrivate->isDefault && isDefault)
    {
	ccsResetToDefault (setting, processChanged);
	return TRUE;
    }

    if (!strcmp (sPrivate->value->value.asString, data))
	return TRUE;

    if (sPrivate->isDefault)
	copyFromDefault (setting);

    free (sPrivate->value->value.asString);

    sPrivate->value->value.asString = strdup (data);

    if (processChanged)
	ccsContextAddChangedSetting (ccsPluginGetContext (sPrivate->parent), setting);

    return TRUE;
}

Bool
ccsSettingSetColorDefault (CCSSetting * setting, CCSSettingColorValue data, Bool processChanged)
{
    SETTING_PRIV (setting)

    if (sPrivate->type != TypeColor)
	return FALSE;

    CCSSettingColorValue defValue = sPrivate->defaultValue.value.asColor;

    Bool isDefault = ccsIsEqualColor (defValue, data);

    if (sPrivate->isDefault && isDefault)
	return TRUE;

    if (!sPrivate->isDefault && isDefault)
    {
	ccsResetToDefault (setting, processChanged);
	return TRUE;
    }

    if (ccsIsEqualColor (sPrivate->value->value.asColor, data))
	return TRUE;

    if (sPrivate->isDefault)
	copyFromDefault (setting);

    sPrivate->value->value.asColor = data;

    if (processChanged)
	ccsContextAddChangedSetting (ccsPluginGetContext (sPrivate->parent), setting);

    return TRUE;
}

Bool
ccsSettingSetMatchDefault (CCSSetting * setting, const char *data, Bool processChanged)
{
    SETTING_PRIV (setting);

    if (sPrivate->type != TypeMatch)
	return FALSE;

    if (!data)
	return FALSE;

    Bool isDefault = strcmp (sPrivate->defaultValue.value.asMatch, data) == 0;

    if (sPrivate->isDefault && isDefault)
	return TRUE;

    if (!sPrivate->isDefault && isDefault)
    {
	ccsResetToDefault (setting, processChanged);
	return TRUE;
    }

    if (!strcmp (sPrivate->value->value.asMatch, data))
	return TRUE;

    if (sPrivate->isDefault)
	copyFromDefault (setting);

    free (sPrivate->value->value.asMatch);

    sPrivate->value->value.asMatch = strdup (data);

    if (processChanged)
	ccsContextAddChangedSetting (ccsPluginGetContext (sPrivate->parent), setting);

    return TRUE;
}

Bool
ccsSettingSetKeyDefault (CCSSetting * setting, CCSSettingKeyValue data, Bool processChanged)
{
    SETTING_PRIV (setting);

    if (sPrivate->type != TypeKey)
	return FALSE;

    CCSSettingKeyValue defValue = sPrivate->defaultValue.value.asKey;

    Bool isDefault = ccsIsEqualKey (data, defValue);

    if (sPrivate->isDefault && isDefault)
	return TRUE;

    if (!sPrivate->isDefault && isDefault)
    {
	ccsResetToDefault (setting, processChanged);
	return TRUE;
    }

    if (ccsIsEqualKey (sPrivate->value->value.asKey, data))
	return TRUE;

    if (sPrivate->isDefault)
	copyFromDefault (setting);

    sPrivate->value->value.asKey.keysym = data.keysym;
    sPrivate->value->value.asKey.keyModMask = data.keyModMask;

    if (processChanged)
	ccsContextAddChangedSetting (ccsPluginGetContext (sPrivate->parent), setting);

    return TRUE;
}

Bool
ccsSettingSetButtonDefault (CCSSetting * setting, CCSSettingButtonValue data, Bool processChanged)
{
    SETTING_PRIV (setting);

    if (sPrivate->type != TypeButton)
	return FALSE;

    CCSSettingButtonValue defValue = sPrivate->defaultValue.value.asButton;

    Bool isDefault = ccsIsEqualButton (data, defValue);

    if (sPrivate->isDefault && isDefault)
	return TRUE;

    if (!sPrivate->isDefault && isDefault)
    {
	ccsResetToDefault (setting, processChanged);
	return TRUE;
    }

    if (ccsIsEqualButton (sPrivate->value->value.asButton, data))
	return TRUE;

    if (sPrivate->isDefault)
	copyFromDefault (setting);

    sPrivate->value->value.asButton.button = data.button;
    sPrivate->value->value.asButton.buttonModMask = data.buttonModMask;
    sPrivate->value->value.asButton.edgeMask = data.edgeMask;

    if (processChanged)
	ccsContextAddChangedSetting (ccsPluginGetContext (sPrivate->parent), setting);

    return TRUE;
}

Bool
ccsSettingSetEdgeDefault (CCSSetting * setting, unsigned int data, Bool processChanged)
{
    SETTING_PRIV (setting)

    if (sPrivate->type != TypeEdge)
	return FALSE;

    Bool isDefault = (data == sPrivate->defaultValue.value.asEdge);

    if (sPrivate->isDefault && isDefault)
	return TRUE;

    if (!sPrivate->isDefault && isDefault)
    {
	ccsResetToDefault (setting, processChanged);
	return TRUE;
    }

    if (sPrivate->value->value.asEdge == data)
	return TRUE;

    if (sPrivate->isDefault)
	copyFromDefault (setting);

    sPrivate->value->value.asEdge = data;

    if (processChanged)
	ccsContextAddChangedSetting (ccsPluginGetContext (sPrivate->parent), setting);

    return TRUE;
}

Bool
ccsSettingSetBellDefault (CCSSetting * setting, Bool data, Bool processChanged)
{
    SETTING_PRIV (setting);

    if (sPrivate->type != TypeBell)
	return FALSE;

    Bool isDefault = (data == sPrivate->defaultValue.value.asBool);

    if (sPrivate->isDefault && isDefault)
	return TRUE;

    if (!sPrivate->isDefault && isDefault)
    {
	ccsResetToDefault (setting, processChanged);
	return TRUE;
    }

    if (sPrivate->value->value.asBell == data)
	return TRUE;

    if (sPrivate->isDefault)
	copyFromDefault (setting);

    sPrivate->value->value.asBell = data;

    if (processChanged)
	ccsContextAddChangedSetting (ccsPluginGetContext (sPrivate->parent), setting);

    return TRUE;
}

static CCSSettingValueList
ccsCopyList (CCSSettingValueList l1, CCSSetting * setting)
{
    CCSSettingValueList l2 = NULL;

    SETTING_PRIV (setting)

    while (l1)
    {
	CCSSettingValue *value = calloc (1, sizeof (CCSSettingValue));
	if (!value)
	    return l2;

	value->refCount = 1;
	value->parent = setting;
	value->isListChild = TRUE;

	switch (sPrivate->info.forList.listType)
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
	    return NULL;
	    break;
	}

	l2 = ccsSettingValueListAppend (l2, value);
	l1 = l1->next;
    }

    return l2;
}

Bool
ccsSettingSetListDefault (CCSSetting * setting, CCSSettingValueList data, Bool processChanged)
{
    SETTING_PRIV (setting)

    if (sPrivate->type != TypeList)
	return FALSE;

    Bool isDefault = ccsCompareLists (sPrivate->defaultValue.value.asList, data,
				      sPrivate->info.forList);

    /* Don't need to worry about default values
     * when processChanged is off since use of that
     * API wants direct access to ths list for
     * temporary storage */
    if (!processChanged)
    {
	if (sPrivate->isDefault && isDefault)
	{
	    return TRUE;
	}

	if (!sPrivate->isDefault && isDefault)
	{
	    ccsResetToDefault (setting, processChanged);
	    return TRUE;
	}
    }

    if (ccsCompareLists (sPrivate->value->value.asList, data,
			 sPrivate->info.forList))
    {
	return TRUE;
    }

    if (sPrivate->isDefault)
	copyFromDefault (setting);

    ccsSettingValueListFree (sPrivate->value->value.asList, TRUE);

    sPrivate->value->value.asList = ccsCopyList (data, setting);

    if ((strcmp (sPrivate->name, "active_plugins") == 0) &&
	(strcmp (ccsPluginGetName (sPrivate->parent), "core") == 0) && processChanged)
    {
	CCSStringList list;

	list = ccsGetStringListFromValueList (sPrivate->value->value.asList);
	ccsSetActivePluginList (ccsPluginGetContext (sPrivate->parent), list);
	ccsStringListFree (list, TRUE);
    }

    if (processChanged)
	ccsContextAddChangedSetting (ccsPluginGetContext (sPrivate->parent), setting);

    return TRUE;
}

Bool
ccsSettingSetValueDefault (CCSSetting * setting, CCSSettingValue * data, Bool processChanged)
{
    SETTING_PRIV (setting);

    switch (sPrivate->type)
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
ccsSettingGetIntDefault (CCSSetting * setting, int *data)
{
    SETTING_PRIV (setting);

    if (sPrivate->type != TypeInt)
	return FALSE;

    *data = sPrivate->value->value.asInt;
    return TRUE;
}

Bool
ccsSettingGetFloatDefault (CCSSetting * setting, float *data)
{
    SETTING_PRIV (setting)

    if (sPrivate->type != TypeFloat)
	return FALSE;

    *data = sPrivate->value->value.asFloat;
    return TRUE;
}

Bool
ccsSettingGetBoolDefault (CCSSetting * setting, Bool * data)
{
    SETTING_PRIV (setting)

    if (sPrivate->type != TypeBool)
	return FALSE;

    *data = sPrivate->value->value.asBool;
    return TRUE;
}

Bool
ccsSettingGetStringDefault (CCSSetting * setting, char **data)
{
    SETTING_PRIV (setting)

    if (sPrivate->type != TypeString)
	return FALSE;

    *data = sPrivate->value->value.asString;
    return TRUE;
}

Bool
ccsSettingGetColorDefault (CCSSetting * setting, CCSSettingColorValue * data)
{
    SETTING_PRIV (setting)

    if (sPrivate->type != TypeColor)
	return TRUE;

    *data = sPrivate->value->value.asColor;
    return TRUE;
}

Bool
ccsSettingGetMatchDefault (CCSSetting * setting, char **data)
{
    SETTING_PRIV (setting)

    if (sPrivate->type != TypeMatch)
	return FALSE;

    *data = sPrivate->value->value.asMatch;
    return TRUE;
}

Bool
ccsSettingGetKeyDefault (CCSSetting * setting, CCSSettingKeyValue * data)
{
    SETTING_PRIV (setting)

    if (sPrivate->type != TypeKey)
	return FALSE;

    *data = sPrivate->value->value.asKey;
    return TRUE;
}

Bool
ccsSettingGetButtonDefault (CCSSetting * setting, CCSSettingButtonValue * data)
{
    SETTING_PRIV (setting)

    if (sPrivate->type != TypeButton)
	return FALSE;

    *data = sPrivate->value->value.asButton;
    return TRUE;
}

Bool
ccsSettingGetEdgeDefault (CCSSetting * setting, unsigned int * data)
{
    SETTING_PRIV (setting)

    if (sPrivate->type != TypeEdge)
	return FALSE;

    *data = sPrivate->value->value.asEdge;
    return TRUE;
}

Bool
ccsSettingGetBellDefault (CCSSetting * setting, Bool * data)
{
    SETTING_PRIV (setting)

    if (sPrivate->type != TypeBell)
	return FALSE;

    *data = sPrivate->value->value.asBell;
    return TRUE;
}

Bool
ccsSettingGetListDefault (CCSSetting * setting, CCSSettingValueList * data)
{
    SETTING_PRIV (setting)

    if (sPrivate->type != TypeList)
	return FALSE;

    *data = sPrivate->value->value.asList;
    return TRUE;
}

Bool ccsGetInt (CCSSetting *setting,
		int        *data)
{
    return (*(GET_INTERFACE (CCSSettingInterface, setting))->settingGetInt) (setting, data);
}

Bool ccsGetFloat (CCSSetting *setting,
		  float      *data)
{
    return (*(GET_INTERFACE (CCSSettingInterface, setting))->settingGetFloat) (setting, data);
}

Bool ccsGetBool (CCSSetting *setting,
		 Bool       *data)
{
    return (*(GET_INTERFACE (CCSSettingInterface, setting))->settingGetBool) (setting, data);
}

Bool ccsGetString (CCSSetting *setting,
		   char       **data)
{
    return (*(GET_INTERFACE (CCSSettingInterface, setting))->settingGetString) (setting, data);
}

Bool ccsGetColor (CCSSetting           *setting,
		  CCSSettingColorValue *data)
{
    return (*(GET_INTERFACE (CCSSettingInterface, setting))->settingGetColor) (setting, data);
}

Bool ccsGetMatch (CCSSetting *setting,
		  char       **data)
{
    return (*(GET_INTERFACE (CCSSettingInterface, setting))->settingGetMatch) (setting, data);
}

Bool ccsGetKey (CCSSetting         *setting,
		CCSSettingKeyValue *data)
{
    return (*(GET_INTERFACE (CCSSettingInterface, setting))->settingGetKey) (setting, data);
}

Bool ccsGetButton (CCSSetting            *setting,
		   CCSSettingButtonValue *data)
{
    return (*(GET_INTERFACE (CCSSettingInterface, setting))->settingGetButton) (setting, data);
}

Bool ccsGetEdge (CCSSetting  *setting,
		 unsigned int *data)
{
    return (*(GET_INTERFACE (CCSSettingInterface, setting))->settingGetEdge) (setting, data);
}

Bool ccsGetBell (CCSSetting *setting,
		 Bool       *data)
{
    return (*(GET_INTERFACE (CCSSettingInterface, setting))->settingGetBell) (setting, data);
}

Bool ccsGetList (CCSSetting          *setting,
		 CCSSettingValueList *data)
{
    return (*(GET_INTERFACE (CCSSettingInterface, setting))->settingGetList) (setting, data);
}

Bool ccsSetInt (CCSSetting *setting,
		int        data,
		Bool	   processChanged)
{
    return (*(GET_INTERFACE (CCSSettingInterface, setting))->settingSetInt) (setting, data, processChanged);
}

Bool ccsSetFloat (CCSSetting *setting,
		  float      data,
		  Bool	     processChanged)
{
    return (*(GET_INTERFACE (CCSSettingInterface, setting))->settingSetFloat) (setting, data, processChanged);
}

Bool ccsSetBool (CCSSetting *setting,
		 Bool       data,
		 Bool	    processChanged)
{
    return (*(GET_INTERFACE (CCSSettingInterface, setting))->settingSetBool) (setting, data, processChanged);
}

Bool ccsSetString (CCSSetting *setting,
		   const char *data,
		   Bool	      processChanged)
{
    return (*(GET_INTERFACE (CCSSettingInterface, setting))->settingSetString) (setting, data, processChanged);
}

Bool ccsSetColor (CCSSetting           *setting,
		  CCSSettingColorValue data,
		  Bool		       processChanged)
{
    return (*(GET_INTERFACE (CCSSettingInterface, setting))->settingSetColor) (setting, data, processChanged);
}

Bool ccsSetMatch (CCSSetting *setting,
		  const char *data,
		  Bool	     processChanged)
{
    return (*(GET_INTERFACE (CCSSettingInterface, setting))->settingSetMatch) (setting, data, processChanged);
}

Bool ccsSetKey (CCSSetting         *setting,
		CCSSettingKeyValue data,
		Bool		   processChanged)
{
    return (*(GET_INTERFACE (CCSSettingInterface, setting))->settingSetKey) (setting, data, processChanged);
}

Bool ccsSetButton (CCSSetting            *setting,
		   CCSSettingButtonValue data,
		   Bool			 processChanged)
{
    return (*(GET_INTERFACE (CCSSettingInterface, setting))->settingSetButton) (setting, data, processChanged);
}

Bool ccsSetEdge (CCSSetting   *setting,
		 unsigned int data,
		 Bool	      processChanged)
{
    return (*(GET_INTERFACE (CCSSettingInterface, setting))->settingSetEdge) (setting, data, processChanged);
}

Bool ccsSetBell (CCSSetting *setting,
		 Bool       data,
		 Bool	    processChanged)
{
    return (*(GET_INTERFACE (CCSSettingInterface, setting))->settingSetBell) (setting, data, processChanged);
}

Bool ccsSetList (CCSSetting          *setting,
		 CCSSettingValueList data,
		 Bool	 processChanged)
{
    return (*(GET_INTERFACE (CCSSettingInterface, setting))->settingSetList) (setting, data, processChanged);
}

Bool ccsSetValue (CCSSetting      *setting,
		  CCSSettingValue *data,
		  Bool		  processChanged)
{
    return (*(GET_INTERFACE (CCSSettingInterface, setting))->settingSetValue) (setting, data, processChanged);
}

void ccsResetToDefault (CCSSetting * setting, Bool processChanged)
{
    (*(GET_INTERFACE (CCSSettingInterface, setting))->settingResetToDefault) (setting, processChanged);
}

Bool ccsSettingIsIntegrated (CCSSetting *setting)
{
    return (*(GET_INTERFACE (CCSSettingInterface, setting))->settingIsIntegrated) (setting);
}

Bool ccsSettingIsReadOnly (CCSSetting *setting)
{
    return (*(GET_INTERFACE (CCSSettingInterface, setting))->settingIsReadOnly) (setting);
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
ccsGetActivePluginListDefault (CCSContext * context)
{
    CONTEXT_PRIV (context);

    CCSPluginList rv = NULL;
    CCSPluginList l = cPrivate->plugins;

    while (l)
    {
	PLUGIN_PRIV (l->data);
	if (pPrivate->active && strcmp (ccsPluginGetName (l->data), "ccp"))
	{
	    rv = ccsPluginListAppend (rv, l->data);
	}

	l = l->next;
    }

    return rv;
}

CCSPluginList
ccsGetActivePluginList (CCSContext *context)
{
    return (*(GET_INTERFACE (CCSContextInterface, context))->contextGetActivePluginList) (context);
}

static CCSPlugin *
findPluginInList (CCSPluginList list, char *name)
{
    if (!name || !strlen (name))
	return NULL;

    while (list)
    {
	if (!strcmp (ccsPluginGetName (list->data), name))
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
ccsGetSortedPluginStringListDefault (CCSContext * context)
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
	CCSStringList l = ccsPluginGetLoadAfter (plugins[i].plugin);
	while (l)
	{
	    p = findPluginInList (ap, ((CCSString *)l->data)->value);

	    if (p && !ccsPluginListFind (plugins[i].after, p))
		plugins[i].after = ccsPluginListAppend (plugins[i].after, p);

	    l = l->next;
	}

	l = ccsPluginGetRequiresPlugins (plugins[i].plugin);
	while (l)
	{
	    Bool found = FALSE;
	    p = findPluginInList (ap, ((CCSString *)l->data)->value);

	    CCSStringList l2 = ccsPluginGetLoadBefore (plugins[i].plugin);
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

	l = ccsPluginGetLoadBefore (plugins[i].plugin);
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
		strcmp (ccsPluginGetName (plugins[i].plugin), "bench") == 0)
		continue;

	    found = TRUE;
	    removed++;
	    p = plugins[i].plugin;
	    plugins[i].plugin = NULL;

	    for (j = 0; j < len; j++)
		plugins[j].after =
		    ccsPluginListRemove (plugins[j].after, p, FALSE);

	    strPluginName = calloc (1, sizeof (CCSString));
	    
	    strPluginName->value = strdup (ccsPluginGetName (p));
	    strPluginName->refCount = 1;

	    rv = ccsStringListAppend (rv, strPluginName);
	}

	if (!found)
	    error = TRUE;
    }

    if (error)
    {
	ccsError ("Unable to generate sorted plugin list");

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

CCSStringList
ccsGetSortedPluginStringList (CCSContext *context)
{
    return (*(GET_INTERFACE (CCSContextInterface, context))->contextGetSortedPluginStringList) (context);
}

char *
ccsGetBackendDefault (CCSContext * context)
{
    if (!context)
	return NULL;

    CONTEXT_PRIV (context);

    if (!cPrivate->backend)
	return NULL;

    return cPrivate->backend->vTable->name;
}

char *
ccsGetBackend (CCSContext *context)
{
    return (*(GET_INTERFACE (CCSContextInterface, context))->contextGetBackend) (context);
}

Bool
ccsGetIntegrationEnabledDefault (CCSContext * context)
{
    if (!context)
	return FALSE;

    CONTEXT_PRIV (context);

    return cPrivate->deIntegration;
}

Bool
ccsGetIntegrationEnabled (CCSContext *context)
{
    if (!context)
	return FALSE;

    return (*(GET_INTERFACE (CCSContextInterface, context))->contextGetIntegrationEnabled) (context);
}

char *
ccsGetProfileDefault (CCSContext * context)
{
    if (!context)
	return NULL;

    CONTEXT_PRIV (context);

    return cPrivate->profile;
}

char *
ccsGetProfile (CCSContext *context)
{
    if (!context)
	return NULL;

    return (*(GET_INTERFACE (CCSContextInterface, context))->contextGetProfile) (context);
}

Bool
ccsGetPluginListAutoSortDefault (CCSContext * context)
{
    if (!context)
	return FALSE;

    CONTEXT_PRIV (context);

    return cPrivate->pluginListAutoSort;
}

Bool
ccsGetPluginListAutoSort (CCSContext *context)
{
    if (!context)
	return FALSE;

    return (*(GET_INTERFACE (CCSContextInterface, context))->contextGetPluginListAutoSort) (context);
}

void
ccsSetIntegrationEnabledDefault (CCSContext * context, Bool value)
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

void
ccsSetIntegrationEnabled (CCSContext *context, Bool value)
{
    (*(GET_INTERFACE (CCSContextInterface, context))->contextSetIntegrationEnabled) (context, value);
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
ccsSetPluginListAutoSortDefault (CCSContext * context, Bool value)
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
ccsSetPluginListAutoSort (CCSContext *context, Bool value)
{
    (*(GET_INTERFACE (CCSContextInterface, context))->contextSetPluginListAutoSort) (context, value);
}

void
ccsSetProfileDefault (CCSContext * context, char *name)
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
ccsSetProfile (CCSContext *context, char *name)
{
    (*(GET_INTERFACE (CCSContextInterface, context))->contextSetProfile) (context, name);
}

void
ccsProcessEventsDefault (CCSContext * context, unsigned int flags)
{
    if (!context)
	return;

    CONTEXT_PRIV (context);

    ccsCheckFileWatches ();

    if (cPrivate->backend && cPrivate->backend->vTable->executeEvents)
	(*cPrivate->backend->vTable->executeEvents) (flags);
}

void
ccsProcessEvents (CCSContext *context, unsigned int flags)
{
    if (!context)
	return;

    (*(GET_INTERFACE (CCSContextInterface, context))->contextProcessEvents) (context, flags);
}

void
ccsReadSettingsDefault (CCSContext * context)
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

    CCSPluginList pl = cPrivate->plugins;
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
ccsReadSettings (CCSContext *context)
{
    if (!context)
	return;

    (*(GET_INTERFACE (CCSContextInterface, context))->contextReadSettings) (context);
}

void
ccsReadPluginSettingsDefault (CCSPlugin * plugin)
{
    if (!plugin || !ccsPluginGetContext (plugin))
	return;

    CONTEXT_PRIV (ccsPluginGetContext (plugin));

    if (!cPrivate->backend)
	return;

    if (!cPrivate->backend->vTable->readSetting)
	return;

    if (cPrivate->backend->vTable->readInit)
	if (!(*cPrivate->backend->vTable->readInit) (ccsPluginGetContext (plugin)))
	    return;

    PLUGIN_PRIV (plugin);

    CCSSettingList sl = pPrivate->settings;
    while (sl)
    {
	(*cPrivate->backend->vTable->readSetting) (ccsPluginGetContext (plugin), sl->data);
	sl = sl->next;
    }

    if (cPrivate->backend->vTable->readDone)
	(*cPrivate->backend->vTable->readDone) (ccsPluginGetContext (plugin));
}

void
ccsReadPluginSettings (CCSPlugin *plugin)
{
    return (*(GET_INTERFACE (CCSPluginInterface, plugin))->pluginReadPluginSettings) (plugin);
}

void
ccsWriteSettingsDefault (CCSContext * context)
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

    CCSPluginList pl = cPrivate->plugins;
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

    cPrivate->changedSettings =
	ccsSettingListFree (cPrivate->changedSettings, FALSE);
}

void
ccsWriteSettings (CCSContext *context)
{
    if (!context)
	return;

    (*(GET_INTERFACE (CCSContextInterface, context))->contextWriteSettings) (context);
}

void
ccsWriteChangedSettingsDefault (CCSContext * context)
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

    if (ccsSettingListLength (cPrivate->changedSettings))
    {
	CCSSettingList l = cPrivate->changedSettings;

	while (l)
	{
	    (*cPrivate->backend->vTable->writeSetting) (context, l->data);
	    l = l->next;
	}
    }

    if (cPrivate->backend->vTable->writeDone)
	(*cPrivate->backend->vTable->writeDone) (context);

    cPrivate->changedSettings =
	ccsSettingListFree (cPrivate->changedSettings, FALSE);
}

void
ccsWriteChangedSettings (CCSContext *context)
{
    if (!context)
	return;

    (*(GET_INTERFACE (CCSContextInterface, context))->contextWriteChangedSettings) (context);
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
    CONTEXT_PRIV (ccsPluginGetContext (plugin));

    pPrivate->active = value;

    if (cPrivate->pluginListAutoSort)
	ccsWriteAutoSortedPluginList (ccsPluginGetContext (plugin));

    return TRUE;
}

CCSPluginConflictList
ccsCanEnablePluginDefault (CCSContext * context, CCSPlugin * plugin)
{
    CCSPluginConflictList list = NULL;
    CCSPluginList pl, pl2;
    CCSStringList sl;

    /* look if the plugin to be loaded requires a plugin not present */
    sl = ccsPluginGetRequiresPlugins (plugin);

    CONTEXT_PRIV (context);

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
    sl = ccsPluginGetRequiresFeatures (plugin);

    while (sl)
    {
	pl = cPrivate->plugins;
	pl2 = NULL;

	while (pl)
	{
	    CCSStringList featureList = ccsPluginGetProvidesFeatures (pl->data);

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
	    if (ccsPluginIsActive (context, ccsPluginGetName (pl->data)))
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
    sl = ccsPluginGetProvidesFeatures (plugin);
    while (sl)
    {
	pl = cPrivate->plugins;
	CCSPluginConflict *conflict = NULL;

	while (pl)
	{
	    if (ccsPluginIsActive (context, ccsPluginGetName (pl->data)))
	    {
		CCSStringList featureList = ccsPluginGetProvidesFeatures (pl->data);

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
    sl = ccsPluginGetProvidesFeatures (plugin);
    while (sl)
    {
	pl = cPrivate->plugins;
	CCSPluginConflict *conflict = NULL;

	while (pl)
	{
	    if (ccsPluginIsActive (context, ccsPluginGetName (pl->data)))
	    {
		CCSStringList featureList = ccsPluginGetProvidesFeatures (pl->data);
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
    sl = ccsPluginGetConflictPlugins (plugin);

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
ccsCanEnablePlugin (CCSContext *context, CCSPlugin *plugin)
{
    return (*(GET_INTERFACE (CCSContextInterface, context))->contextCanEnablePlugin) (context, plugin);
}

CCSPluginConflictList
ccsCanDisablePluginDefault (CCSContext * context, CCSPlugin * plugin)
{
    CCSPluginConflictList list = NULL;
    CCSPluginConflict *conflict = NULL;
    CCSPluginList pl;
    CCSStringList sl;

    CONTEXT_PRIV (context);

    /* look if the plugin to be unloaded is required by another plugin */
    pl = cPrivate->plugins;

    for (; pl; pl = pl->next)
    {
	CCSStringList pluginList;

	if (pl->data == plugin)
	    continue;

	if (!ccsPluginIsActive (context, ccsPluginGetName (pl->data)))
	    continue;

	pluginList = ccsPluginGetRequiresPlugins (pl->data);

	while (pluginList)
	{
	    if (strcmp (ccsPluginGetName (plugin), ((CCSString *)pluginList->data)->value) == 0)
	    {
		if (!conflict)
		{
		    conflict = calloc (1, sizeof (CCSPluginConflict));
		    conflict->refCount = 1;
		    if (conflict)
		    {
			conflict->value = strdup (ccsPluginGetName (plugin));
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
    sl = ccsPluginGetProvidesFeatures (plugin);
    while (sl)
    {
	pl = cPrivate->plugins;
	for (; pl; pl = pl->next)
	{
	    CCSStringList pluginList;

	    if (pl->data == plugin)
		continue;

	    if (!ccsPluginIsActive (context, ccsPluginGetName (pl->data)))
		continue;

	    pluginList = ccsPluginGetRequiresFeatures (pl->data);

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

CCSPluginConflictList
ccsCanDisablePlugin (CCSContext *context, CCSPlugin *plugin)
{
    return (*(GET_INTERFACE (CCSContextInterface, context))->contextCanDisablePlugin) (context, plugin);
}

CCSStringList
ccsGetExistingProfilesDefault (CCSContext * context)
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

CCSStringList
ccsGetExistingProfiles (CCSContext *context)
{
    if (!context)
	return NULL;

    return (*(GET_INTERFACE (CCSContextInterface, context))->contextGetExistingProfiles) (context);
}

void
ccsDeleteProfileDefault (CCSContext * context, char *name)
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

void
ccsDeleteProfile (CCSContext *context, char *name)
{
    if (!context)
	return;

    (*(GET_INTERFACE (CCSContextInterface, context))->contextDeleteProfile) (context, name);
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
    char *override_backend = getenv ("LIBCOMPIZCONFIG_BACKEND_PATH");
    char *backenddir;

    if (override_backend && strlen (override_backend))
    {
	if (asprintf (&backenddir, "%s",
		      override_backend) == -1)
	    backenddir = NULL;

	if (backenddir)
	{
	    getBackendInfoFromDir (&rv, backenddir);
	    free (backenddir);
	}
    }

    if (home && strlen (home))
    {
	if (asprintf (&backenddir, "%s/.compizconfig/backends", home) == -1)
	    backenddir = NULL;

	if (backenddir)
	{
	    getBackendInfoFromDir (&rv, backenddir);
	    free (backenddir);
	}
    }

    if (asprintf (&backenddir, "%s/compizconfig/backends", LIBDIR) == -1)
	backenddir = NULL;

    if (backenddir)
    {
	getBackendInfoFromDir (&rv, backenddir);
	free (backenddir);
    }
    return rv;
}

Bool
ccsExportToFileDefault (CCSContext *context,
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

    CONTEXT_PRIV (context);

    for (p = cPrivate->plugins; p; p = p->next)
    {
	plugin = p->data;
	PLUGIN_PRIV (plugin);

	if (!pPrivate->loaded)
	    ccsLoadPluginSettings (plugin);

	for (s = pPrivate->settings; s; s = s->next)
	{
	    setting = s->data;

	    if (skipDefaults && ccsSettingGetIsDefault (setting))
		continue;

	    if (asprintf (&keyName, "s%d_%s",
			  cPrivate->screenNum, ccsSettingGetName (setting)) == -1)
		return FALSE;

	    switch (ccsSettingGetType (setting))
 	    {
 	    case TypeBool:
		ccsIniSetBool (exportFile, ccsPluginGetName (plugin), keyName,
			       ccsSettingGetValue (setting)->value.asBool);
 		break;
 	    case TypeInt:
		ccsIniSetInt (exportFile, ccsPluginGetName (plugin), keyName,
			      ccsSettingGetValue (setting)->value.asInt);
 		break;
 	    case TypeFloat:
		ccsIniSetFloat (exportFile, ccsPluginGetName (plugin), keyName,
				ccsSettingGetValue (setting)->value.asFloat);
 		break;
 	    case TypeString:
		ccsIniSetString (exportFile, ccsPluginGetName (plugin), keyName,
				 ccsSettingGetValue (setting)->value.asString);
 		break;
 	    case TypeKey:
		ccsIniSetKey (exportFile, ccsPluginGetName (plugin), keyName,
			      ccsSettingGetValue (setting)->value.asKey);
 		break;
 	    case TypeButton:
		ccsIniSetButton (exportFile, ccsPluginGetName (plugin), keyName,
				 ccsSettingGetValue (setting)->value.asButton);
 		break;
 	    case TypeEdge:
		ccsIniSetEdge (exportFile, ccsPluginGetName (plugin), keyName,
			       ccsSettingGetValue (setting)->value.asEdge);
 		break;
 	    case TypeBell:
		ccsIniSetBell (exportFile, ccsPluginGetName (plugin), keyName,
			       ccsSettingGetValue (setting)->value.asBell);
 		break;
 	    case TypeColor:
		ccsIniSetColor (exportFile, ccsPluginGetName (plugin), keyName,
				ccsSettingGetValue (setting)->value.asColor);
 		break;
 	    case TypeMatch:
		ccsIniSetString (exportFile, ccsPluginGetName (plugin), keyName,
				 ccsSettingGetValue (setting)->value.asMatch);
 		break;
 	    case TypeList:
		ccsIniSetList (exportFile, ccsPluginGetName (plugin), keyName,
			       ccsSettingGetValue (setting)->value.asList,
			       ccsSettingGetInfo (setting)->forList.listType);
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

Bool
ccsExportToFile (CCSContext *context, const char *fileName, Bool skipDefaults)
{
    if (!context)
	return FALSE;

    return (*(GET_INTERFACE (CCSContextInterface, context))->contextExportToFile) (context, fileName, skipDefaults);
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
    char         *sectionName = strdup (ccsPluginGetName (ccsSettingGetParent (setting)));
    char         *iniValue = NULL;

    CONTEXT_PRIV (context);

    if (asprintf (&keyName, "+s%d_%s", cPrivate->screenNum, ccsSettingGetName (setting)) == -1)
	return FALSE;

    if (ccsIniGetString (dict, sectionName, keyName, &iniValue))
    {
	CCSSetting *newSetting = malloc (sizeof (CCSSetting));

	if (!newSetting)
	    return FALSE;

	ccsObjectInit (newSetting, &ccsDefaultObjectAllocator);

	copySetting (setting, newSetting);

	switch (ccsSettingGetType (newSetting))
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

	    if (strcmp (ccsSettingGetName (s), ccsSettingGetName (newSetting)) == 0)
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
    char         *sectionName = strdup (ccsPluginGetName (ccsSettingGetParent (setting)));
    char         *iniValue = NULL;

    CONTEXT_PRIV (context);

    if (asprintf (&keyName, "-s%d_%s", cPrivate->screenNum, ccsSettingGetName (setting)) == -1)
	return FALSE;

    if (ccsIniGetString (dict, sectionName, keyName, &iniValue))
    {
	CCSSetting *newSetting = malloc (sizeof (CCSSetting));

	if (!newSetting)
	    return FALSE;

	ccsObjectInit (newSetting, &ccsDefaultObjectAllocator);

	copySetting (setting, newSetting);

	switch (ccsSettingGetType (newSetting))
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

	    if (strcmp (ccsSettingGetName (s), ccsSettingGetName (newSetting)) == 0)
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
    CONTEXT_PRIV (context);

    IniDictionary      *dict = ccsIniOpen (upgrade->file);
    CCSPluginList      pl = cPrivate->plugins;
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
	
	setting = ccsFindSetting (ccsSettingGetParent (tempSetting), ccsSettingGetName (tempSetting));
	
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

    sl = upgrade->addValueSettings;
    
    while (sl)
    {
	CCSSetting *tempSetting = (CCSSetting *) sl->data;
	CCSSetting *setting;
	
	setting = ccsFindSetting (ccsSettingGetParent (tempSetting), ccsSettingGetName (tempSetting));
	
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

    sl = upgrade->replaceFromValueSettings;
    
    while (sl)
    {
	CCSSetting *tempSetting = (CCSSetting *) sl->data;
	CCSSetting *setting;
	
	setting = ccsFindSetting (ccsSettingGetParent (tempSetting), ccsSettingGetName (tempSetting));
	
	if (setting)
	{
	    if (ccsSettingGetValue (setting) == ccsSettingGetValue (tempSetting))
	    {
		CCSSettingList rl = upgrade->replaceToValueSettings;
		
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
ccsCheckForSettingsUpgradeDefault (CCSContext *context)
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

    if (asprintf (&dupath, "%s/.config/compiz-1/compizconfig/done_upgrades", home) == -1)
	return FALSE;

    completedUpgrades = fopen (dupath, "a+");

    if (!path)
    {
	fclose (completedUpgrades);
	return FALSE;
    }

    nFile = scandir (path, &nameList, upgradeNameFilter, alphasort);
    if (nFile <= 0)
    {
	fclose (completedUpgrades);
	return FALSE;
    }

    if (!completedUpgrades)
    {
	fclose (completedUpgrades);
	ccsWarning ("Error opening done_upgrades");
	return FALSE;
    }

    fseek (completedUpgrades, 0, SEEK_END);
    cuSize = ftell (completedUpgrades);
    rewind (completedUpgrades);

    cuBuffer = calloc (cuSize + 1, sizeof (char));
    cuReadSize = fread (cuBuffer, 1, cuSize, completedUpgrades);

    if (cuReadSize != cuSize)
	ccsWarning ("Couldn't read completed upgrades file!");

    cuBuffer[cuSize] = '\0';

    for (i = 0; i < nFile; i++)
    {
	char		   *matched = strstr (cuBuffer, nameList[i]->d_name);
	CCSSettingsUpgrade *upgrade;
	
	if (matched)
	{
	    ccsDebug ("Skipping upgrade %s", nameList[i]->d_name);
	    continue;
	}

	upgrade = ccsSettingsUpgradeNew (path, nameList[i]->d_name);
	
	ccsDebug ("Processing upgrade %s\nprofile: %s\nnumber: %i\ndomain: %s", nameList[i]->d_name, upgrade->profile, upgrade->num, upgrade->domain);

	ccsProcessUpgrade (context, upgrade);
	ccsWriteChangedSettings (context);
	ccsWriteAutoSortedPluginList (context);
	ccsDebug ("Completed upgrade %s", nameList[i]->d_name);
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
ccsImportFromFileDefault (CCSContext *context,
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

    CONTEXT_PRIV (context);

    for (p = cPrivate->plugins; p; p = p->next)
    {
	plugin = p->data;
	PLUGIN_PRIV (plugin);

	if (!pPrivate->loaded)
	    ccsLoadPluginSettings (plugin);

	for (s = pPrivate->settings; s; s = s->next)
	{
	    setting = s->data;
	    if (!ccsSettingGetIsDefault (setting) && !overwriteNonDefault)
		continue;

	    if (asprintf (&keyName, "s%d_%s",
			  cPrivate->screenNum, ccsSettingGetName (setting)) == -1)
		return FALSE;

	    switch (ccsSettingGetType (setting))
	    {
	    case TypeBool:
		{
		    Bool value;

		    if (ccsIniGetBool (importFile, ccsPluginGetName (plugin),
 				       keyName, &value))
		    {
			ccsSetBool (setting, value, TRUE);
		    }
		}
		break;
	    case TypeInt:
		{
		    int value;

		    if (ccsIniGetInt (importFile, ccsPluginGetName (plugin),
				      keyName, &value))
			ccsSetInt (setting, value, TRUE);
		}
		break;
	    case TypeFloat:
		{
		    float value;

		    if (ccsIniGetFloat (importFile, ccsPluginGetName (plugin),
					keyName, &value))
			ccsSetFloat (setting, value, TRUE);
		}
		break;
	    case TypeString:
		{
		    char *value;

		    if (ccsIniGetString (importFile, ccsPluginGetName (plugin),
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

		    if (ccsIniGetKey (importFile, ccsPluginGetName (plugin),
				      keyName, &value))
			ccsSetKey (setting, value, TRUE);
		}
		break;
	    case TypeButton:
		{
		    CCSSettingButtonValue value;

		    if (ccsIniGetButton (importFile, ccsPluginGetName (plugin),
					 keyName, &value))
			ccsSetButton (setting, value, TRUE);
		}
		break;
	    case TypeEdge:
		{
		    unsigned int value;

		    if (ccsIniGetEdge (importFile, ccsPluginGetName (plugin),
				       keyName, &value))
			ccsSetEdge (setting, value, TRUE);
		}
		break;
	    case TypeBell:
		{
		    Bool value;

		    if (ccsIniGetBell (importFile, ccsPluginGetName (plugin),
				       keyName, &value))
			ccsSetBell (setting, value, TRUE);
		}
		break;
	    case TypeColor:
		{
		    CCSSettingColorValue value;

		    if (ccsIniGetColor (importFile, ccsPluginGetName (plugin),
					keyName, &value))
			ccsSetColor (setting, value, TRUE);
		}
		break;
	    case TypeMatch:
		{
		    char *value;
		    if (ccsIniGetString (importFile, ccsPluginGetName (plugin),
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
		    if (ccsIniGetList (importFile, ccsPluginGetName (plugin),
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

Bool
ccsCheckForSettingsUpgrade (CCSContext *context)
{
    return (*(GET_INTERFACE (CCSContextInterface, context))->contextCheckForSettingsUpgrade) (context);
}

Bool
ccsImportFromFile (CCSContext *context, const char *fileName, Bool overwriteNonDefault)
{
    if (!context)
	return FALSE;

    return (*(GET_INTERFACE (CCSContextInterface, context))->contextImportFromFile) (context, fileName, overwriteNonDefault);
}

char *
ccsPluginGetNameDefault (CCSPlugin *plugin)
{
    PLUGIN_PRIV (plugin);

    return pPrivate->name;
}

char * ccsPluginGetShortDescDefault (CCSPlugin *plugin)
{
    PLUGIN_PRIV (plugin);

    return pPrivate->shortDesc;
}

char * ccsPluginGetLongDescDefault (CCSPlugin *plugin)
{
    PLUGIN_PRIV (plugin);

    return pPrivate->longDesc;
}

char * ccsPluginGetHintsDefault (CCSPlugin *plugin)
{
    PLUGIN_PRIV (plugin);

    return pPrivate->hints;
}

char * ccsPluginGetCategoryDefault (CCSPlugin *plugin)
{
    PLUGIN_PRIV (plugin);

    return pPrivate->category;
}

CCSStringList ccsPluginGetLoadAfterDefault (CCSPlugin *plugin)
{
    PLUGIN_PRIV (plugin);

    return pPrivate->loadAfter;
}

CCSStringList ccsPluginGetLoadBeforeDefault (CCSPlugin *plugin)
{
    PLUGIN_PRIV (plugin);

    return pPrivate->loadBefore;
}

CCSStringList ccsPluginGetRequiresPluginsDefault (CCSPlugin *plugin)
{
    PLUGIN_PRIV (plugin);

    return pPrivate->requiresPlugin;
}

CCSStringList ccsPluginGetConflictPluginsDefault (CCSPlugin *plugin)
{
    PLUGIN_PRIV (plugin);

    return pPrivate->conflictPlugin;
}

CCSStringList ccsPluginGetProvidesFeaturesDefault (CCSPlugin *plugin)
{
    PLUGIN_PRIV (plugin);

    return pPrivate->providesFeature;
}

void * ccsPluginGetProvidesFeaturesBindable (CCSPlugin *plugin)
{
    return (void *) ccsPluginGetProvidesFeatures (plugin);
}

CCSStringList ccsPluginGetRequiresFeaturesDefault (CCSPlugin *plugin)
{
    PLUGIN_PRIV (plugin);

    return pPrivate->requiresFeature;
}

void * ccsPluginGetPrivatePtrDefault (CCSPlugin *plugin)
{
    PLUGIN_PRIV (plugin);

    return pPrivate->privatePtr;
}

void ccsPluginSetPrivatePtrDefault (CCSPlugin *plugin, void *ptr)
{
    PLUGIN_PRIV (plugin);

    pPrivate->privatePtr = ptr;
}

CCSContext * ccsPluginGetContextDefault (CCSPlugin *plugin)
{
    PLUGIN_PRIV (plugin);

    return pPrivate->context;
}

/* CCSPlugin accessor functions */
char * ccsPluginGetName (CCSPlugin *plugin)
{
    return (*(GET_INTERFACE (CCSPluginInterface, plugin))->pluginGetName) (plugin);
}

char * ccsPluginGetShortDesc (CCSPlugin *plugin)
{
    return (*(GET_INTERFACE (CCSPluginInterface, plugin))->pluginGetShortDesc) (plugin);
}

char * ccsPluginGetLongDesc (CCSPlugin *plugin)
{
    return (*(GET_INTERFACE (CCSPluginInterface, plugin))->pluginGetLongDesc) (plugin);
}

char * ccsPluginGetHints (CCSPlugin *plugin)
{
    return (*(GET_INTERFACE (CCSPluginInterface, plugin))->pluginGetHints) (plugin);
}

char * ccsPluginGetCategory (CCSPlugin *plugin)
{
    return (*(GET_INTERFACE (CCSPluginInterface, plugin))->pluginGetCategory) (plugin);
}

CCSStringList ccsPluginGetLoadAfter (CCSPlugin *plugin)
{
    return (*(GET_INTERFACE (CCSPluginInterface, plugin))->pluginGetLoadAfter) (plugin);
}

CCSStringList ccsPluginGetLoadBefore (CCSPlugin *plugin)
{
    return (*(GET_INTERFACE (CCSPluginInterface, plugin))->pluginGetLoadBefore) (plugin);
}

CCSStringList ccsPluginGetRequiresPlugins (CCSPlugin *plugin)
{
    return (*(GET_INTERFACE (CCSPluginInterface, plugin))->pluginGetRequiresPlugins) (plugin);
}

CCSStringList ccsPluginGetConflictPlugins (CCSPlugin *plugin)
{
    return (*(GET_INTERFACE (CCSPluginInterface, plugin))->pluginGetConflictPlugins) (plugin);
}

CCSStringList ccsPluginGetProvidesFeatures (CCSPlugin *plugin)
{
    return (*(GET_INTERFACE (CCSPluginInterface, plugin))->pluginGetProvidesFeatures) (plugin);
}

CCSStringList ccsPluginGetRequiresFeatures (CCSPlugin *plugin)
{
    return (*(GET_INTERFACE (CCSPluginInterface, plugin))->pluginGetRequiresFeatures) (plugin);
}

void * ccsPluginGetPrivatePtr (CCSPlugin *plugin)
{
    return (*(GET_INTERFACE (CCSPluginInterface, plugin))->pluginGetPrivatePtr) (plugin);
}

void ccsPluginSetPrivatePtr (CCSPlugin *plugin, void *ptr)
{
    (*(GET_INTERFACE (CCSPluginInterface, plugin))->pluginSetPrivatePtr) (plugin, ptr);
}

CCSContext * ccsPluginGetContext (CCSPlugin *plugin)
{
    return (*(GET_INTERFACE (CCSPluginInterface, plugin))->pluginGetContext) (plugin);
}

CCSSettingList ccsGetPluginSettingsDefault (CCSPlugin *plugin)
{
    PLUGIN_PRIV (plugin);

    if (!pPrivate->loaded)
	ccsLoadPluginSettings (plugin);

    return pPrivate->settings;
}

CCSSettingList ccsGetPluginSettings (CCSPlugin *plugin)
{
    return (*(GET_INTERFACE (CCSPluginInterface, plugin))->pluginGetPluginSettings) (plugin);
}

CCSGroupList ccsGetPluginGroupsDefault (CCSPlugin *plugin)
{
    PLUGIN_PRIV (plugin);

    if (!pPrivate->loaded)
	ccsLoadPluginSettings (plugin);

    return pPrivate->groups;
}

CCSGroupList ccsGetPluginGroups (CCSPlugin *plugin)
{
    return (*(GET_INTERFACE (CCSPluginInterface, plugin))->pluginGetPluginGroups) (plugin);
}

char * ccsSettingGetName (CCSSetting *setting)
{
    return (*(GET_INTERFACE (CCSSettingInterface, setting))->settingGetName) (setting);
}

char * ccsSettingGetShortDesc (CCSSetting *setting)

{
    return (*(GET_INTERFACE (CCSSettingInterface, setting))->settingGetShortDesc) (setting);
}

char * ccsSettingGetLongDesc (CCSSetting *setting)
{
    return (*(GET_INTERFACE (CCSSettingInterface, setting))->settingGetLongDesc) (setting);
}

CCSSettingType ccsSettingGetType (CCSSetting *setting)
{
    return (*(GET_INTERFACE (CCSSettingInterface, setting))->settingGetType) (setting);
}

CCSSettingInfo * ccsSettingGetInfo (CCSSetting *setting)
{
    return (*(GET_INTERFACE (CCSSettingInterface, setting))->settingGetInfo) (setting);
}

char * ccsSettingGetGroup (CCSSetting *setting)
{
    return (*(GET_INTERFACE (CCSSettingInterface, setting))->settingGetGroup) (setting);
}

char * ccsSettingGetSubGroup (CCSSetting *setting)
{
    return (*(GET_INTERFACE (CCSSettingInterface, setting))->settingGetSubGroup) (setting);
}

char * ccsSettingGetHints (CCSSetting *setting)
{
    return (*(GET_INTERFACE (CCSSettingInterface, setting))->settingGetHints) (setting);
}

CCSSettingValue * ccsSettingGetDefaultValue (CCSSetting *setting)
{
    return (*(GET_INTERFACE (CCSSettingInterface, setting))->settingGetDefaultValue) (setting);
}

CCSSettingValue *ccsSettingGetValue (CCSSetting *setting)
{
    return (*(GET_INTERFACE (CCSSettingInterface, setting))->settingGetValue) (setting);
}

Bool ccsSettingGetIsDefault (CCSSetting *setting)
{
    return (*(GET_INTERFACE (CCSSettingInterface, setting))->settingGetIsDefault) (setting);
}

CCSPlugin * ccsSettingGetParent (CCSSetting *setting)
{
    return (*(GET_INTERFACE (CCSSettingInterface, setting))->settingGetParent) (setting);
}

void * ccsSettingGetPrivatePtr (CCSSetting *setting)
{
    return (*(GET_INTERFACE (CCSSettingInterface, setting))->settingGetPrivatePtr) (setting);
}

void ccsSettingSetPrivatePtr (CCSSetting *setting, void *ptr)
{
    return (*(GET_INTERFACE (CCSSettingInterface, setting))->settingSetPrivatePtr) (setting, ptr);
}

Bool ccsSettingGetIsIntegratedDefault (CCSSetting *setting)
{
    if (!setting)
	return FALSE;

    CONTEXT_PRIV (ccsPluginGetContext (ccsSettingGetParent (setting)));

    if (!cPrivate->backend)
	return FALSE;

    if (cPrivate->backend->vTable->getSettingIsIntegrated)
	return (*cPrivate->backend->vTable->getSettingIsIntegrated) (setting);

    return FALSE;
}

Bool ccsSettingGetIsReadOnlyDefault (CCSSetting *setting)
{
    if (!setting)
	return FALSE;

    CONTEXT_PRIV (ccsPluginGetContext (ccsSettingGetParent (setting)));

    if (!cPrivate->backend)
	return FALSE;

    if (cPrivate->backend->vTable->getSettingIsReadOnly)
	return (*cPrivate->backend->vTable->getSettingIsReadOnly) (setting);

    return FALSE;
}

/* Interface for CCSSetting */
char *
ccsSettingGetNameDefault (CCSSetting *setting)
{
    SETTING_PRIV (setting);

    return sPrivate->name;
}

char * ccsSettingGetShortDescDefault (CCSSetting *setting)
{
    SETTING_PRIV (setting);

    return sPrivate->shortDesc;
}

char * ccsSettingGetLongDescDefault (CCSSetting *setting)
{
    SETTING_PRIV (setting);

    return sPrivate->longDesc;
}

CCSSettingType ccsSettingGetTypeDefault (CCSSetting *setting)
{
    SETTING_PRIV (setting);

    return sPrivate->type;
}

CCSSettingInfo * ccsSettingGetInfoDefault (CCSSetting *setting)
{
    SETTING_PRIV (setting);

    return &sPrivate->info;
}

char * ccsSettingGetGroupDefault (CCSSetting *setting)
{
    SETTING_PRIV (setting);

    return sPrivate->group;
}

char * ccsSettingGetSubGroupDefault (CCSSetting *setting)
{
    SETTING_PRIV (setting);

    return sPrivate->subGroup;
}

char * ccsSettingGetHintsDefault (CCSSetting *setting)
{
    SETTING_PRIV (setting);

    return sPrivate->hints;
}

CCSSettingValue * ccsSettingGetDefaultValueDefault (CCSSetting *setting)
{
    SETTING_PRIV (setting);

    return &sPrivate->defaultValue;
}

CCSSettingValue * ccsSettingGetValueDefault (CCSSetting *setting)
{
    SETTING_PRIV (setting);

    return sPrivate->value;
}

Bool ccsSettingGetIsDefaultDefault (CCSSetting *setting)
{
    SETTING_PRIV (setting);

    return sPrivate->isDefault;
}

CCSPlugin * ccsSettingGetParentDefault (CCSSetting *setting)
{
    SETTING_PRIV (setting);

    return sPrivate->parent;
}

void * ccsSettingGetPrivatePtrDefault (CCSSetting *setting)
{
    SETTING_PRIV (setting);

    return sPrivate->privatePtr;
}

void ccsSettingSetPrivatePtrDefault (CCSSetting *setting, void *ptr)
{
    SETTING_PRIV (setting);

    sPrivate->privatePtr = ptr;
}

CCSStrExtensionList ccsGetPluginStrExtensionsDefault (CCSPlugin *plugin)
{
    PLUGIN_PRIV (plugin);

    if (!pPrivate->loaded)
	ccsLoadPluginSettings (plugin);

    return pPrivate->stringExtensions;
}

CCSStrExtensionList ccsGetPluginStrExtensions (CCSPlugin *plugin)
{
    return (*(GET_INTERFACE (CCSPluginInterface, plugin))->pluginGetPluginStrExtensions) (plugin);
}

static  const CCSPluginInterface ccsDefaultPluginInterface =
{
    ccsPluginGetNameDefault,
    ccsPluginGetShortDescDefault,
    ccsPluginGetLongDescDefault,
    ccsPluginGetHintsDefault,
    ccsPluginGetCategoryDefault,
    ccsPluginGetLoadAfterDefault,
    ccsPluginGetLoadBeforeDefault,
    ccsPluginGetRequiresPluginsDefault,
    ccsPluginGetConflictPluginsDefault,
    ccsPluginGetProvidesFeaturesDefault,
    ccsPluginGetRequiresFeaturesDefault,
    ccsPluginGetPrivatePtrDefault,
    ccsPluginSetPrivatePtrDefault,
    ccsPluginGetContextDefault,
    ccsFindSettingDefault,
    ccsGetPluginSettingsDefault,
    ccsGetPluginGroupsDefault,
    ccsReadPluginSettingsDefault,
    ccsGetPluginStrExtensionsDefault
};

static const CCSContextInterface ccsDefaultContextInterface =
{
    ccsContextGetPluginsDefault,
    ccsContextGetCategoriesDefault,
    ccsContextGetChangedSettingsDefault,
    ccsContextGetScreenNumDefault,
    ccsContextAddChangedSettingDefault,
    ccsContextClearChangedSettingsDefault,
    ccsContextStealChangedSettingsDefault,
    ccsContextGetPrivatePtrDefault,
    ccsContextSetPrivatePtrDefault,
    ccsLoadPluginDefault,
    ccsFindPluginDefault,
    ccsPluginIsActiveDefault,
    ccsGetActivePluginListDefault,
    ccsGetSortedPluginStringListDefault,
    ccsSetBackendDefault,
    ccsGetBackendDefault,
    ccsSetIntegrationEnabledDefault,
    ccsSetProfileDefault,
    ccsSetPluginListAutoSortDefault,
    ccsGetProfileDefault,
    ccsGetIntegrationEnabledDefault,
    ccsGetPluginListAutoSortDefault,
    ccsProcessEventsDefault,
    ccsReadSettingsDefault,
    ccsWriteSettingsDefault,
    ccsWriteChangedSettingsDefault,
    ccsExportToFileDefault,
    ccsImportFromFileDefault,
    ccsCanEnablePluginDefault,
    ccsCanDisablePluginDefault,
    ccsGetExistingProfilesDefault,
    ccsDeleteProfileDefault,
    ccsCheckForSettingsUpgradeDefault,
    ccsLoadPluginsDefault
};

static const CCSSettingInterface ccsDefaultSettingInterface =
{
    ccsSettingGetNameDefault,
    ccsSettingGetShortDescDefault,
    ccsSettingGetLongDescDefault,
    ccsSettingGetTypeDefault,
    ccsSettingGetInfoDefault,
    ccsSettingGetGroupDefault,
    ccsSettingGetSubGroupDefault,
    ccsSettingGetHintsDefault,
    ccsSettingGetDefaultValueDefault,
    ccsSettingGetValueDefault,
    ccsSettingGetIsDefaultDefault,
    ccsSettingGetParentDefault,
    ccsSettingGetPrivatePtrDefault,
    ccsSettingSetPrivatePtrDefault,
    ccsSettingSetIntDefault,
    ccsSettingSetFloatDefault,
    ccsSettingSetBoolDefault,
    ccsSettingSetStringDefault,
    ccsSettingSetColorDefault,
    ccsSettingSetMatchDefault,
    ccsSettingSetKeyDefault,
    ccsSettingSetButtonDefault,
    ccsSettingSetEdgeDefault,
    ccsSettingSetBellDefault,
    ccsSettingSetListDefault,
    ccsSettingSetValueDefault,
    ccsSettingGetIntDefault,
    ccsSettingGetFloatDefault,
    ccsSettingGetBoolDefault,
    ccsSettingGetStringDefault,
    ccsSettingGetColorDefault,
    ccsSettingGetMatchDefault,
    ccsSettingGetKeyDefault,
    ccsSettingGetButtonDefault,
    ccsSettingGetEdgeDefault,
    ccsSettingGetBellDefault,
    ccsSettingGetListDefault,
    ccsSettingResetToDefaultDefault,
    ccsSettingGetIsIntegratedDefault,
    ccsSettingGetIsReadOnlyDefault,
    ccsFreeSettingDefault
};

const CCSInterfaceTable ccsDefaultInterfaceTable =
{
    &ccsDefaultContextInterface,
    &ccsDefaultPluginInterface,
    &ccsDefaultSettingInterface
};
