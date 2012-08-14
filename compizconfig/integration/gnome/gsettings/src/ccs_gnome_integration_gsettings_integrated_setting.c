#include <stdlib.h>
#include <string.h>

#include <gio/gio.h>

#include <ccs.h>
#include <ccs-backend.h>
#include <ccs-object.h>

#include <ccs_gsettings_interface.h>
#include <gsettings_shared.h>

#include "ccs_gnome_integration_gsettings_integrated_setting.h"
#include "ccs_gnome_integrated_setting.h"
#include "ccs_gnome_integration_constants.h"


/* CCSGSettingsIntegratedSetting implementation */
typedef struct _CCSGSettingsIntegratedSettingPrivate CCSGSettingsIntegratedSettingPrivate;

struct _CCSGSettingsIntegratedSettingPrivate
{
    CCSGNOMEIntegratedSetting *gnomeIntegratedSetting;
    CCSGSettingsWrapper	      *wrapper;
};

SpecialOptionType
ccsGSettingsIntegratedSettingGetSpecialOptionType (CCSGNOMEIntegratedSetting *setting)
{
    CCSGSettingsIntegratedSettingPrivate *priv = (CCSGSettingsIntegratedSettingPrivate *) ccsObjectGetPrivate (setting);

    return ccsGNOMEIntegratedSettingGetSpecialOptionType (priv->gnomeIntegratedSetting);
}

const char *
ccsGSettingsIntegratedSettingGetGNOMEName (CCSGNOMEIntegratedSetting *setting)
{
    CCSGSettingsIntegratedSettingPrivate *priv = (CCSGSettingsIntegratedSettingPrivate *) ccsObjectGetPrivate (setting);

    return ccsGNOMEIntegratedSettingGetGNOMEName (priv->gnomeIntegratedSetting);
}

CCSSettingValue *
ccsGSettingsIntegratedSettingReadValue (CCSIntegratedSetting *setting, CCSSettingType type)
{
    CCSGSettingsIntegratedSettingPrivate *priv = (CCSGSettingsIntegratedSettingPrivate *) ccsObjectGetPrivate (setting);
    CCSSettingValue		     *v = calloc (1, sizeof (CCSSettingValue));
    const char			     *gnomeKeyName = ccsGNOMEIntegratedSettingGetGNOMEName ((CCSGNOMEIntegratedSetting *) setting);
    char			     *gsettingsTranslatedName = translateKeyForGSettings (gnomeKeyName);

    v->isListChild = FALSE;
    v->parent = NULL;
    v->refCount = 1;

    GVariant *variant = ccsGSettingsWrapperGetValue (priv->wrapper, gsettingsTranslatedName);

    if (!variant)
	free (gsettingsTranslatedName);

    const GVariantType *variantType = G_VARIANT_TYPE (g_variant_get_type_string (variant));

    switch (type)
    {
	case TypeInt:
	    if (!g_variant_type_equal (variantType, G_VARIANT_TYPE_INT32))
	    {
		ccsError ("Expected integer value");
		break;
	    }

	    v->value.asInt = readIntFromVariant (variant);
	    break;
	case TypeBool:
	    if (!g_variant_type_equal (variantType, G_VARIANT_TYPE_BOOLEAN))
	    {
		ccsError ("Expected boolean value");
		break;
	    }

	    v->value.asBool = readBoolFromVariant (variant);
	    break;
	case TypeString:
	{
	    if (!g_variant_type_equal (variantType, G_VARIANT_TYPE_STRING))
	    {
		ccsError ("Expected string value");
		break;
	    }

	    const char *str = readStringFromVariant (variant);
	    v->value.asString = strdup (str ? str : "");
	    break;
	}
	case TypeKey:
	{
	    if (!g_variant_type_equal (variantType, G_VARIANT_TYPE ("as")))
	    {
		ccsError ("Expected array-of-string value");
		break;
	    }

	    gsize len;
	    const gchar **strv = g_variant_get_strv (variant, &len);

	    if (strv)
		v->value.asString = strdup (strv[0] ? strv[0] : "");
	    else
		v->value.asString = strdup ("");

	    g_free (strv);
	    break;
	}
	default:
	    g_assert_not_reached ();
    }

    g_variant_unref (variant);
    free (gsettingsTranslatedName);

    return v;
}

void
ccsGSettingsIntegratedSettingWriteValue (CCSIntegratedSetting *setting, CCSSettingValue *v, CCSSettingType type)
{
    CCSGSettingsIntegratedSettingPrivate *priv = (CCSGSettingsIntegratedSettingPrivate *) ccsObjectGetPrivate (setting);
    const char			     *gnomeKeyName = ccsGNOMEIntegratedSettingGetGNOMEName ((CCSGNOMEIntegratedSetting *) setting);
    char			     *gsettingsTranslatedName = translateKeyForGSettings (gnomeKeyName);

    GVariant *variant = ccsGSettingsWrapperGetValue (priv->wrapper, gsettingsTranslatedName);
    GVariant *newVariant = NULL;

    if (!variant)
    {
	ccsError ("NULL encountered while reading GSettings value");
	free (gsettingsTranslatedName);
	return;
    }

    switch (type)
    {
	case TypeInt:
	    {
		int currentValue = readIntFromVariant (variant);

		if ((currentValue != v->value.asInt))
		    writeIntToVariant (v->value.asInt, &newVariant);
	    }
	    break;
	case TypeBool:
	    {
		gboolean currentValue = readBoolFromVariant (variant);

		if ((currentValue != v->value.asBool))
		    writeBoolToVariant (v->value.asBool, &newVariant);
	    }
	    break;
	case TypeString:
	    {
		char  *newValue = v->value.asString;
		gsize len = 0;
		const gchar *currentValue = g_variant_get_string (variant, &len);

		if (currentValue)
		{
		    if (strcmp (currentValue, newValue) != 0)
			writeStringToVariant (currentValue, &newVariant);
		}
	    }
	case TypeKey:
	    {
		GVariantBuilder strvBuilder;

		g_variant_builder_init (&strvBuilder, G_VARIANT_TYPE ("as"));
		g_variant_builder_add (&strvBuilder, v->value.asString);
		newVariant = g_variant_builder_end (&strvBuilder);

		break;
	    }
	default:
	    g_assert_not_reached ();
	    break;
    }

    /* g_settings_set_value consumes the reference */
    if (newVariant)
	ccsGSettingsWrapperSetValue (priv->wrapper, gsettingsTranslatedName, newVariant);

    g_variant_unref (variant);
    free (gsettingsTranslatedName);
}

const char *
ccsGSettingsIntegratedSettingPluginName (CCSIntegratedSetting *setting)
{
    CCSGSettingsIntegratedSettingPrivate *priv = (CCSGSettingsIntegratedSettingPrivate *) ccsObjectGetPrivate (setting);

    return ccsIntegratedSettingPluginName ((CCSIntegratedSetting *) priv->gnomeIntegratedSetting);
}

const char *
ccsGSettingsIntegratedSettingSettingName (CCSIntegratedSetting *setting)
{
    CCSGSettingsIntegratedSettingPrivate *priv = (CCSGSettingsIntegratedSettingPrivate *) ccsObjectGetPrivate (setting);

    return ccsIntegratedSettingSettingName ((CCSIntegratedSetting *) priv->gnomeIntegratedSetting);
}

CCSSettingType
ccsGSettingsIntegratedSettingGetType (CCSIntegratedSetting *setting)
{
    CCSGSettingsIntegratedSettingPrivate *priv = (CCSGSettingsIntegratedSettingPrivate *) ccsObjectGetPrivate (setting);

    return ccsIntegratedSettingGetType ((CCSIntegratedSetting *) priv->gnomeIntegratedSetting);
}

void
ccsGSettingsIntegratedSettingFree (CCSIntegratedSetting *setting)
{
    CCSGSettingsIntegratedSettingPrivate *priv = (CCSGSettingsIntegratedSettingPrivate *) ccsObjectGetPrivate (setting);

    if (priv->wrapper)
	ccsGSettingsWrapperUnref (priv->wrapper);

    ccsIntegratedSettingUnref ((CCSIntegratedSetting *) priv->gnomeIntegratedSetting);
    ccsObjectFinalize (setting);

    (*setting->object.object_allocation->free_) (setting->object.object_allocation->allocator, setting);
}

const CCSGNOMEIntegratedSettingInterface ccsGSettingsGNOMEIntegratedSettingInterface =
{
    ccsGSettingsIntegratedSettingGetSpecialOptionType,
    ccsGSettingsIntegratedSettingGetGNOMEName
};

const CCSIntegratedSettingInterface ccsGSettingsIntegratedSettingInterface =
{
    ccsGSettingsIntegratedSettingReadValue,
    ccsGSettingsIntegratedSettingWriteValue,
    ccsGSettingsIntegratedSettingPluginName,
    ccsGSettingsIntegratedSettingSettingName,
    ccsGSettingsIntegratedSettingGetType,
    ccsGSettingsIntegratedSettingFree
};

CCSIntegratedSetting *
ccsGSettingsIntegratedSettingNew (CCSGNOMEIntegratedSetting *base,
			      CCSGSettingsWrapper	*wrapper,
			      CCSObjectAllocationInterface *ai)
{
    CCSIntegratedSetting *setting = (*ai->calloc_) (ai->allocator, 1, sizeof (CCSIntegratedSetting));

    if (!setting)
	return NULL;

    CCSGSettingsIntegratedSettingPrivate *priv = (*ai->calloc_) (ai->allocator, 1, sizeof (CCSGSettingsIntegratedSettingPrivate));

    if (!priv)
    {
	(*ai->free_) (ai->allocator, priv);
	return NULL;
    }

    priv->gnomeIntegratedSetting = base;
    priv->wrapper = wrapper;

    ccsGSettingsWrapperRef (priv->wrapper);

    ccsObjectInit (setting, ai);
    ccsObjectSetPrivate (setting, (CCSPrivate *) priv);
    ccsObjectAddInterface (setting, (const CCSInterface *) &ccsGSettingsIntegratedSettingInterface, GET_INTERFACE_TYPE (CCSIntegratedSettingInterface));
    ccsObjectAddInterface (setting, (const CCSInterface *) &ccsGSettingsGNOMEIntegratedSettingInterface, GET_INTERFACE_TYPE (CCSGNOMEIntegratedSettingInterface));
    ccsIntegratedSettingRef (setting);

    return setting;
}
