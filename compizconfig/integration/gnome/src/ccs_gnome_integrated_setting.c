#include <stdlib.h>
#include <string.h>

#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include <gconf/gconf-value.h>

#include <ccs.h>
#include <ccs-backend.h>
#include <ccs-object.h>

#include "ccs_gnome_integrated_setting.h"
#include "ccs_gnome_integration_constants.h"

INTERFACE_TYPE (CCSGNOMEIntegratedSettingInterface);

SpecialOptionType
ccsGNOMEIntegratedSettingGetSpecialOptionType (CCSGNOMEIntegratedSetting *setting)
{
    return (*(GET_INTERFACE (CCSGNOMEIntegratedSettingInterface, setting))->getSpecialOptionType) (setting);
}

const char *
ccsGNOMEIntegratedSettingGetGNOMEName (CCSGNOMEIntegratedSetting *setting)
{
    return (*(GET_INTERFACE (CCSGNOMEIntegratedSettingInterface, setting))->getGNOMEName) (setting);
}

/* CCSGNOMEIntegratedSettingDefaultImpl implementation */

typedef struct _CCSGNOMEIntegratedSettingDefaultImplPrivate CCSGNOMEIntegratedSettingDefaultImplPrivate;

struct _CCSGNOMEIntegratedSettingDefaultImplPrivate
{
    SpecialOptionType type;
    const char	      *gnomeName;
    CCSIntegratedSetting *sharedIntegratedSetting;
};

Bool
ccsGNOMEIntegrationFindSettingsMatchingPredicate (CCSIntegratedSetting *setting,
						  void		       *userData)
{
    const char *findGnomeName = (const char *) userData;
    const char *gnomeNameOfSetting = ccsGNOMEIntegratedSettingGetGNOMEName ((CCSGNOMEIntegratedSetting *) setting);

    if (strcmp (findGnomeName, gnomeNameOfSetting) == 0)
	return TRUE;

    return FALSE;
}

SpecialOptionType
ccsGNOMEIntegratedSettingGetSpecialOptionDefault (CCSGNOMEIntegratedSetting *setting)
{
    CCSGNOMEIntegratedSettingDefaultImplPrivate *priv = (CCSGNOMEIntegratedSettingDefaultImplPrivate *) ccsObjectGetPrivate (setting);

    return priv->type;
}

const char *
ccsGNOMEIntegratedSettingGetGNOMENameDefault (CCSGNOMEIntegratedSetting *setting)
{
    CCSGNOMEIntegratedSettingDefaultImplPrivate *priv = (CCSGNOMEIntegratedSettingDefaultImplPrivate *) ccsObjectGetPrivate (setting);

    return priv->gnomeName;
}

CCSSettingValue *
ccsGNOMEIntegratedSettingReadValue (CCSIntegratedSetting *setting, CCSSettingType type)
{
    CCSGNOMEIntegratedSettingDefaultImplPrivate *priv = (CCSGNOMEIntegratedSettingDefaultImplPrivate *) ccsObjectGetPrivate (setting);

    return ccsIntegratedSettingReadValue (priv->sharedIntegratedSetting, type);
}

void
ccsGNOMEIntegratedSettingWriteValue (CCSIntegratedSetting *setting, CCSSettingValue *value, CCSSettingType type)
{
    CCSGNOMEIntegratedSettingDefaultImplPrivate *priv = (CCSGNOMEIntegratedSettingDefaultImplPrivate *) ccsObjectGetPrivate (setting);

    ccsIntegratedSettingWriteValue (priv->sharedIntegratedSetting, value, type);
}

const char *
ccsGNOMEIntegratedSettingPluginName (CCSIntegratedSetting *setting)
{
    CCSGNOMEIntegratedSettingDefaultImplPrivate *priv = (CCSGNOMEIntegratedSettingDefaultImplPrivate *) ccsObjectGetPrivate (setting);

    return ccsIntegratedSettingPluginName (priv->sharedIntegratedSetting);
}

const char *
ccsGNOMEIntegratedSettingSettingName (CCSIntegratedSetting *setting)
{
    CCSGNOMEIntegratedSettingDefaultImplPrivate *priv = (CCSGNOMEIntegratedSettingDefaultImplPrivate *) ccsObjectGetPrivate (setting);

    return ccsIntegratedSettingSettingName (priv->sharedIntegratedSetting);
}

CCSSettingType
ccsGNOMEIntegratedSettingGetType (CCSIntegratedSetting *setting)
{
    CCSGNOMEIntegratedSettingDefaultImplPrivate *priv = (CCSGNOMEIntegratedSettingDefaultImplPrivate *) ccsObjectGetPrivate (setting);

    return ccsIntegratedSettingGetType (priv->sharedIntegratedSetting);
}

void
ccsGNOMEIntegratedSettingFree (CCSIntegratedSetting *setting)
{
    CCSGNOMEIntegratedSettingDefaultImplPrivate *priv = (CCSGNOMEIntegratedSettingDefaultImplPrivate *) ccsObjectGetPrivate (setting);

    ccsIntegratedSettingUnref (priv->sharedIntegratedSetting);
    ccsObjectFinalize (setting);

    (*setting->object.object_allocation->free_) (setting->object.object_allocation->allocator, setting);
}

CCSGNOMEIntegratedSettingInterface ccsGNOMEIntegratedSettingDefaultImplInterface =
{
    ccsGNOMEIntegratedSettingGetSpecialOptionDefault,
    ccsGNOMEIntegratedSettingGetGNOMENameDefault
};

const CCSIntegratedSettingInterface ccsGNOMEIntegratedSettingInterface =
{
    ccsGNOMEIntegratedSettingReadValue,
    ccsGNOMEIntegratedSettingWriteValue,
    ccsGNOMEIntegratedSettingPluginName,
    ccsGNOMEIntegratedSettingSettingName,
    ccsGNOMEIntegratedSettingGetType,
    ccsGNOMEIntegratedSettingFree
};

CCSGNOMEIntegratedSetting *
ccsGNOMEIntegratedSettingNew (CCSIntegratedSetting *base,
			      SpecialOptionType    type,
			      const char	   *gnomeName,
			      CCSObjectAllocationInterface *ai)
{
    CCSGNOMEIntegratedSetting *setting = (*ai->calloc_) (ai->allocator, 1, sizeof (CCSGNOMEIntegratedSetting));

    if (!setting)
	return NULL;

    CCSGNOMEIntegratedSettingDefaultImplPrivate *priv = (*ai->calloc_) (ai->allocator, 1, sizeof (CCSGNOMEIntegratedSettingDefaultImplPrivate));

    if (!priv)
    {
	(*ai->free_) (ai->allocator, setting);
	return NULL;
    }

    priv->sharedIntegratedSetting = base;
    priv->gnomeName = gnomeName;
    priv->type = type;

    ccsObjectInit (setting, ai);
    ccsObjectSetPrivate (setting, (CCSPrivate *) priv);
    ccsObjectAddInterface (setting, (const CCSInterface *) &ccsGNOMEIntegratedSettingInterface, GET_INTERFACE_TYPE (CCSIntegratedSettingInterface));
    ccsObjectAddInterface (setting, (const CCSInterface *) &ccsGNOMEIntegratedSettingDefaultImplInterface, GET_INTERFACE_TYPE (CCSGNOMEIntegratedSettingInterface));
    ccsIntegratedSettingRef ((CCSIntegratedSetting *) setting);

    return setting;
}
