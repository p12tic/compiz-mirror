#include <glib-object.h>

#include <gio/gio.h>

#include "gwd-settings-writable-interface.h"
#include "gwd-settings-storage-interface.h"
#include "gwd-settings-storage-gsettings.h"

#define GWD_SETTINGS_STORAGE_GSETTINGS(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GWD_TYPE_SETTINGS_STORAGE_GSETTINGS, GWDSettingsStorageGSettings));
#define GWD_SETTINGS_STORAGE_GSETTINGS_CLASS(obj) (G_TYPE_CHECK_CLASS_CAST ((obj), GWD_TYPE_SETTINGS_STORAGE_GSETTINGS, GWDSettingsStorageGSettingsClass));
#define GWD_IS_MOCK_SETTINGS(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GWD_TYPE_SETTINGS_STORAGE_GSETTINGS));
#define GWD_IS_MOCK_SETTINGS_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((obj), GWD_TYPE_SETTINGS_STORAGE_GSETTINGS));
#define GWD_SETTINGS_STORAGE_GSETTINGS_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GWD_TYPE_SETTINGS_STORAGE_GSETTINGS, GWDSettingsStorageGSettingsClass));

typedef struct _GWDSettingsStorageGSettings
{
    GObject parent;
} GWDSettingsStorageGSettings;

typedef struct _GWDSettingsStorageGSettingsClass
{
    GObjectClass parent_class;
} GWDSettingsStorageGSettingsClass;

static void gwd_settings_storage_gsettings_interface_init (GWDSettingsStorageInterface *interface);

G_DEFINE_TYPE_WITH_CODE (GWDSettingsStorageGSettings, gwd_settings_storage_gsettings, G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (GWD_TYPE_SETTINGS_STORAGE_INTERFACE,
						gwd_settings_storage_gsettings_interface_init))

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GWD_TYPE_SETTINGS_STORAGE_GSETTINGS, GWDSettingsStorageGSettingsPrivate))

enum
{
    GWD_SETTINGS_STORAGE_GSETTINGS_PROPERTY_DESKTOP_GSETTINGS = 1,
    GWD_SETTINGS_STORAGE_GSETTINGS_PROPERTY_MUTTER_GSETTINGS  = 2,
    GWD_SETTINGS_STORAGE_GSETTINGS_PROPERTY_GWD_GSETTINGS     = 3,
    GWD_SETTINGS_STORAGE_GSETTINGS_PROPERTY_WRITABLE_SETTINGS = 4
};

const guint GWD_SETTINGS_STORAGE_GSETTINGS_N_CONSTRUCTION_PARAMS = 4;

typedef struct _GWDSettingsStorageGSettingsPrivate
{
    GSettings *desktop;
    GSettings *mutter;
    GSettings *gwd;
    GWDSettingsWritable *writable;
} GWDSettingsStorageGSettingsPrivate;

gboolean gwd_settings_storage_gsettings_update_use_tooltips (GWDSettingsStorage *settings)
{
    GWDSettingsStorageGSettings	       *storage = GWD_SETTINGS_STORAGE_GSETTINGS (settings);
    GWDSettingsStorageGSettingsPrivate *priv = GET_PRIVATE (storage);

    return gwd_settings_writable_use_tooltips_changed (priv->writable,
						       g_settings_get_boolean (priv->gwd, "use-tooltips"));
}

gboolean gwd_settings_storage_gsettings_update_draggable_border_width (GWDSettingsStorage *settings)
{
    return FALSE;
}

gboolean gwd_settings_storage_gsettings_update_attach_modal_dialogs (GWDSettingsStorage *settings)
{
    return FALSE;
}

gboolean gwd_settings_storage_gsettings_update_blur (GWDSettingsStorage *settings)
{
    return FALSE;
}

gboolean gwd_settings_storage_gsettings_update_metacity_theme (GWDSettingsStorage *settings)
{
    return FALSE;
}

gboolean gwd_settings_storage_gsettings_update_opacity (GWDSettingsStorage *settings)
{
    return FALSE;
}

gboolean gwd_settings_storage_gsettings_update_button_layout (GWDSettingsStorage *settings)
{
    return FALSE;
}

gboolean gwd_settings_storage_gsettings_update_font (GWDSettingsStorage *settings)
{
    return FALSE;
}

gboolean gwd_settings_storage_gsettings_update_titlebar_actions (GWDSettingsStorage *settings)
{
    return FALSE;
}

static void gwd_settings_storage_gsettings_interface_init (GWDSettingsStorageInterface *interface)
{
    interface->update_use_tooltips = gwd_settings_storage_gsettings_update_use_tooltips;
    interface->update_draggable_border_width = gwd_settings_storage_gsettings_update_draggable_border_width;
    interface->update_attach_modal_dialogs = gwd_settings_storage_gsettings_update_attach_modal_dialogs;
    interface->update_blur = gwd_settings_storage_gsettings_update_blur;
    interface->update_metacity_theme = gwd_settings_storage_gsettings_update_metacity_theme;
    interface->update_opacity = gwd_settings_storage_gsettings_update_opacity;
    interface->update_button_layout = gwd_settings_storage_gsettings_update_button_layout;
    interface->update_font = gwd_settings_storage_gsettings_update_font;
    interface->update_titlebar_actions = gwd_settings_storage_gsettings_update_titlebar_actions;
}

static void gwd_settings_storage_gsettings_set_property (GObject *object,
						     guint   property_id,
						     const GValue  *value,
						     GParamSpec *pspec)
{
    GWDSettingsStorageGSettingsPrivate *priv = GET_PRIVATE (object);

    switch (property_id)
    {
	case GWD_SETTINGS_STORAGE_GSETTINGS_PROPERTY_DESKTOP_GSETTINGS:
	    if (priv->desktop)
		g_object_unref (priv->desktop);

	    priv->desktop = g_value_dup_object (value);
	    break;
	case GWD_SETTINGS_STORAGE_GSETTINGS_PROPERTY_MUTTER_GSETTINGS:
	    if (priv->mutter)
		g_object_unref (priv->mutter);

	    priv->mutter = g_value_dup_object (value);
	    break;
	case GWD_SETTINGS_STORAGE_GSETTINGS_PROPERTY_GWD_GSETTINGS:
	    if (priv->gwd)
		g_object_unref (priv->gwd);

	    priv->gwd = g_value_dup_object (value);
	    break;
	case GWD_SETTINGS_STORAGE_GSETTINGS_PROPERTY_WRITABLE_SETTINGS:
	    priv->writable = g_value_get_pointer (value);
	    break;
	default:
	    g_assert_not_reached ();
    }
}

static void gwd_settings_storage_gsettings_dispose (GObject *object)
{
    GWDSettingsStorageGSettingsPrivate *priv = GET_PRIVATE (object);

    G_OBJECT_CLASS (gwd_settings_storage_gsettings_parent_class)->dispose (object);

    if (priv->desktop)
	g_object_unref (priv->desktop);

    if (priv->mutter)
	g_object_unref (priv->mutter);

    if (priv->gwd)
	g_object_unref (priv->gwd);
}

static void gwd_settings_storage_gsettings_finalize (GObject *object)
{
    G_OBJECT_CLASS (gwd_settings_storage_gsettings_parent_class)->finalize (object);
}

static void gwd_settings_storage_gsettings_class_init (GWDSettingsStorageGSettingsClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (klass, sizeof (GWDSettingsStorageGSettingsPrivate));

    object_class->dispose = gwd_settings_storage_gsettings_dispose;
    object_class->finalize = gwd_settings_storage_gsettings_finalize;
    object_class->set_property = gwd_settings_storage_gsettings_set_property;

    GParamSpec * properties[] =
    {
	NULL,
	g_param_spec_object ("desktop-gsettings",
			     "org.gnome.desktop.wm.preferences",
			     "GSettings Object for org.gnome.desktop.wm.preferences",
			     G_TYPE_SETTINGS,
			     G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY),
	g_param_spec_object ("mutter-gsettings",
			     "org.gnome.mutter",
			     "GSettings Object for org.gnome.mutter",
			     G_TYPE_SETTINGS,
			     G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY),
	g_param_spec_object ("gwd-gsettings",
			     "org.compiz.gwd",
			     "GSettings Object for org.compiz.gwd",
			     G_TYPE_SETTINGS,
			     G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY),
	g_param_spec_pointer ("writable-settings",
			      "GWDWritableSettings",
			      "A GWDWritableSettings object",
			      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY)
    };

    g_object_class_install_properties (object_class,
				       sizeof (properties) / sizeof (properties[0]),
				       properties);
}

void gwd_settings_storage_gsettings_init (GWDSettingsStorageGSettings *self)
{
}

GWDSettingsStorage *
gwd_settings_storage_gsettings_new (GSettings *desktop,
				    GSettings *mutter,
				    GSettings *gwd,
				    GWDSettingsWritable *writable)
{
    GValue desktop_value = G_VALUE_INIT;
    GValue mutter_value = G_VALUE_INIT;
    GValue gwd_value = G_VALUE_INIT;
    GValue writable_value = G_VALUE_INIT;

    g_return_val_if_fail (writable != NULL, NULL);

    g_value_init (&desktop_value, G_TYPE_OBJECT);
    g_value_init (&mutter_value, G_TYPE_OBJECT);
    g_value_init (&gwd_value, G_TYPE_OBJECT);
    g_value_init (&writable_value, G_TYPE_POINTER);

    g_value_take_object (&desktop_value, desktop);
    g_value_take_object (&mutter_value, mutter);
    g_value_take_object (&gwd_value, gwd);
    g_value_set_pointer (&writable_value, writable);

    GParameter param[] =
    {
	{ "desktop-gsettings", desktop_value },
	{ "mutter-gsettings", mutter_value },
	{ "gwd-gsettings", gwd_value },
	{ "writable-settings", writable_value }
    };

    GWDSettingsStorage *storage = GWD_SETTINGS_STORAGE_INTERFACE (g_object_newv (GWD_TYPE_SETTINGS_STORAGE_GSETTINGS,
										 GWD_SETTINGS_STORAGE_GSETTINGS_N_CONSTRUCTION_PARAMS,
										 param));

    g_value_unset (&desktop_value);
    g_value_unset (&mutter_value);
    g_value_unset (&gwd_value);
    g_value_unset (&writable_value);

    return storage;
}
