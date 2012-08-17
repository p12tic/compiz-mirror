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

INTERFACE_TYPE (CCSGNOMEIntegratedSettingInfoInterface);

SpecialOptionType
ccsGNOMEIntegratedSettingInfoGetSpecialOptionType (CCSGNOMEIntegratedSettingInfo *info)
{
    return (*(GET_INTERFACE (CCSGNOMEIntegratedSettingInfoInterface, info))->getSpecialOptionType) (info);
}

const char *
ccsGNOMEIntegratedSettingInfoGetGNOMEName (CCSGNOMEIntegratedSettingInfo *info)
{
    return (*(GET_INTERFACE (CCSGNOMEIntegratedSettingInfoInterface, info))->getGNOMEName) (info);
}

/* CCSGNOMEIntegratedSettingDefaultImpl implementation */

typedef struct _CCSGNOMEIntegratedSettingInfoDefaultImplPrivate CCSGNOMEIntegratedSettingInfoDefaultImplPrivate;

struct _CCSGNOMEIntegratedSettingInfoDefaultImplPrivate
{
    SpecialOptionType type;
    const char	      *gnomeName;
    CCSIntegratedSettingInfo *sharedIntegratedSettingInfo;
};

SpecialOptionType
ccsGNOMEIntegratedSettingGetSpecialOptionDefault (CCSGNOMEIntegratedSettingInfo *info)
{
    CCSGNOMEIntegratedSettingInfoDefaultImplPrivate *priv = (CCSGNOMEIntegratedSettingInfoDefaultImplPrivate *) ccsObjectGetPrivate (info);

    return priv->type;
}

const char *
ccsGNOMEIntegratedSettingGetGNOMENameDefault (CCSGNOMEIntegratedSettingInfo *info)
{
    CCSGNOMEIntegratedSettingInfoDefaultImplPrivate *priv = (CCSGNOMEIntegratedSettingInfoDefaultImplPrivate *) ccsObjectGetPrivate (info);

    return priv->gnomeName;
}

const char *
ccsGNOMEIntegratedSettingInfoPluginName (CCSIntegratedSettingInfo *info)
{
    CCSGNOMEIntegratedSettingInfoDefaultImplPrivate *priv = (CCSGNOMEIntegratedSettingInfoDefaultImplPrivate *) ccsObjectGetPrivate (info);

    return ccsIntegratedSettingInfoPluginName (priv->sharedIntegratedSettingInfo);
}

const char *
ccsGNOMEIntegratedSettingInfoSettingName (CCSIntegratedSettingInfo *info)
{
    CCSGNOMEIntegratedSettingInfoDefaultImplPrivate *priv = (CCSGNOMEIntegratedSettingInfoDefaultImplPrivate *) ccsObjectGetPrivate (info);

    return ccsIntegratedSettingInfoSettingName (priv->sharedIntegratedSettingInfo);
}

CCSSettingType
ccsGNOMEIntegratedSettingInfoGetType (CCSIntegratedSettingInfo *info)
{
    CCSGNOMEIntegratedSettingInfoDefaultImplPrivate *priv = (CCSGNOMEIntegratedSettingInfoDefaultImplPrivate *) ccsObjectGetPrivate (info);

    return ccsIntegratedSettingInfoGetType (priv->sharedIntegratedSettingInfo);
}

void
ccsGNOMEIntegratedSettingInfoFree (CCSIntegratedSettingInfo *info)
{
    CCSGNOMEIntegratedSettingInfoDefaultImplPrivate *priv = (CCSGNOMEIntegratedSettingInfoDefaultImplPrivate *) ccsObjectGetPrivate (info);

    ccsIntegratedSettingInfoUnref (priv->sharedIntegratedSettingInfo);
    ccsObjectFinalize (info);

    (*info->object.object_allocation->free_) (info->object.object_allocation->allocator, info);
}

CCSGNOMEIntegratedSettingInfoInterface ccsGNOMEIntegratedSettingInfoDefaultImplInterface =
{
    ccsGNOMEIntegratedSettingGetSpecialOptionDefault,
    ccsGNOMEIntegratedSettingGetGNOMENameDefault
};

const CCSIntegratedSettingInfoInterface ccsGNOMEIntegratedSettingInfoInterface =
{
    ccsGNOMEIntegratedSettingInfoPluginName,
    ccsGNOMEIntegratedSettingInfoSettingName,
    ccsGNOMEIntegratedSettingInfoGetType,
    ccsGNOMEIntegratedSettingInfoFree
};

CCSGNOMEIntegratedSettingInfo *
ccsGNOMEIntegratedSettingInfoNew (CCSIntegratedSettingInfo *base,
				  SpecialOptionType    type,
				  const char	   *gnomeName,
				  CCSObjectAllocationInterface *ai)
{
    CCSGNOMEIntegratedSettingInfo *info = (*ai->calloc_) (ai->allocator, 1, sizeof (CCSGNOMEIntegratedSettingInfo));

    if (!info)
	return NULL;

    CCSGNOMEIntegratedSettingInfoDefaultImplPrivate *priv = (*ai->calloc_) (ai->allocator, 1, sizeof (CCSGNOMEIntegratedSettingInfoDefaultImplPrivate));

    if (!priv)
    {
	(*ai->free_) (ai->allocator, info);
	return NULL;
    }

    priv->sharedIntegratedSettingInfo = base;
    priv->gnomeName = gnomeName;
    priv->type = type;

    ccsObjectInit (info, ai);
    ccsObjectSetPrivate (info, (CCSPrivate *) priv);
    ccsObjectAddInterface (info, (const CCSInterface *) &ccsGNOMEIntegratedSettingInfoInterface, GET_INTERFACE_TYPE (CCSIntegratedSettingInfoInterface));
    ccsObjectAddInterface (info, (const CCSInterface *) &ccsGNOMEIntegratedSettingInfoDefaultImplInterface, GET_INTERFACE_TYPE (CCSGNOMEIntegratedSettingInfoInterface));
    ccsIntegratedSettingInfoRef ((CCSIntegratedSettingInfo *) info);

    return info;
}
