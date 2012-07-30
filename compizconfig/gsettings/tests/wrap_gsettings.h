#ifndef _COMPIZ_COMPIZCONFIG_GSETTINGS_WRAP_GSETTINGS
#define _COMPIZ_COMPIZCONFIG_GSETTINGS_WRAP_GSETTINGS

#include <glib-object.h>

G_BEGIN_DECLS

#define COMPIZCONFIG_GSETTINGS_WRAP_GSETTINGS(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), COMPIZCONFIG_GSETTINGS_TYPE_WRAP_GSETTINGS, CCSGSettingsWrapGSettings))

#define COMPIZCONFIG_GSETTINGS_WRAP_GSETTINGS_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE((inst), COMPIZCONFIG_GSETTINGS_TYPE_WRAP_GSETTINGS, CCSGSettingsWrapGSettingsInterface))

#define COMPIZCONFIG_GSETTINGS_TYPE_WRAP_GSETTINGS (compizconfig_gsettings_wrap_gsettings_get_type())

GType compizconfig_gsettings_wrap_gsettings_get_type (void);

typedef struct _CCSGSettingsWrapGSettings CCSGSettingsWrapGSettings; /* dummy typedef */
typedef struct _CCSGSettingsWrapGSettingsInterface CCSGSettingsWrapGSettingsInterface;

struct _CCSGSettingsWrapGSettingsInterface {
  GTypeInterface parent;

};

enum
{
    COMPIZCONFIG_GSETTINGS_WRAP_GSETTINGS_PROPERTY_SCHEMA_NAME = 1,
    COMPIZCONFIG_GSETTINGS_WRAP_GSETTINGS_PROPERTY_NUM = 2
};

#define COMPIZCONFIG_GSETTINGS_TYPE_MOCK_WRAP_GSETTINGS (compizconfig_gsettings_mock_wrap_gsettings_get_type ())

GType compizconfig_gsettings_mock_wrap_gsettings_get_type ();

/**
 * compizconfig_gsettings_wrap_gsettings_new:
 * @type: (in) (transfer none): a GType of the object type to be created
 * @name: (in) (transfer none): schema name
 * Return value: (transfer full): a new CCSGSettingsWrapGSettings
 */
CCSGSettingsWrapGSettings * compizconfig_gsettings_wrap_gsettings_new (GType type, const gchar *name);

G_END_DECLS

#endif
