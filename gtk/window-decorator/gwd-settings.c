#include <glib-object.h>

#include "gwd-settings.h"
#include "gwd-settings-interface.h"
#include "gwd-settings-writable-interface.h"
#include "decoration.h"

#define GWD_SETTINGS_IMPL(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GWD_TYPE_SETTINGS_IMPL, GWDSettingsImpl));
#define GWD_SETTINGS_IMPL_CLASS(obj) (G_TYPE_CHECK_CLASS_CAST ((obj), GWD_TYPE_SETTINGS_IMPL, GWDSettingsImplClass));
#define GWD_IS_SETTINGS_IMPL(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GWD_TYPE_SETTINGS_IMPL));
#define GWD_IS_SETTINGS_IMPL_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((obj), GWD_TYPE_SETTINGS_IMPL));
#define GWD_SETTINGS_IMPL_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GWD_TYPE_SETTINGS_IMPL, GWDSettingsImplClass));

typedef struct _GWDSettingsImpl
{
    GObject parent;
} GWDSettingsImpl;

typedef struct _GWDSettingsImplClass
{
    GObjectClass parent_class;
} GWDSettingsImplClass;

static void gwd_settings_interface_init (GWDSettingsInterface *interface);
static void gwd_settings_writable_interface_init (GWDSettingsWritableInterface *interface);

G_DEFINE_TYPE_WITH_CODE (GWDSettingsImpl, gwd_settings_impl, G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (GWD_TYPE_SETTINGS_INTERFACE,
						gwd_settings_interface_init)
			 G_IMPLEMENT_INTERFACE (GWD_TYPE_WRITABLE_SETTINGS_INTERFACE,
						gwd_settings_writable_interface_init))

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GWD_TYPE_SETTINGS_IMPL, GWDSettingsImplPrivate))

enum
{
    GWD_SETTINGS_IMPL_PROPERTY_ACTIVE_SHADOW = 1,
    GWD_SETTINGS_IMPL_PROPERTY_INACTIVE_SHADOW = 2,
    GWD_SETTINGS_IMPL_PROPERTY_USE_TOOLTIPS = 3,
    GWD_SETTINGS_IMPL_PROPERTY_DRAGGABLE_BORDER_WIDTH = 4,
    GWD_SETTINGS_IMPL_PROPERTY_ATTACH_MODAL_DIALOGS = 5,
    GWD_SETTINGS_IMPL_PROPERTY_BLUR_CHANGED = 6,
    GWD_SETTINGS_IMPL_PROPERTY_METACITY_THEME = 7,
    GWD_SETTINGS_IMPL_PROPERTY_ACTIVE_OPACITY = 8,
    GWD_SETTINGS_IMPL_PROPERTY_INACTIVE_OPACITY = 9,
    GWD_SETTINGS_IMPL_PROPERTY_ACTIVE_SHADE_OPACITY = 10,
    GWD_SETTINGS_IMPL_PROPERTY_INACTIVE_SHADE_OPACITY = 11,
    GWD_SETTINGS_IMPL_PROPERTY_BUTTON_LAYOUT = 12
};


typedef struct _GWDSettingsImplPrivate
{
    decor_shadow_options_t active_shadow;
    decor_shadow_options_t inactive_shadow;
    gboolean		   use_tooltips;
    gint		   draggable_border_width;
    gboolean		   attach_modal_dialogs;
    gint		   blur_type;
    gchar		   *metacity_theme;
    gdouble		   metacity_active_opacity;
    gdouble		   metacity_inactive_opacity;
    gboolean		   metacity_active_shade_opacity;
    gboolean		   metacity_inactive_shade_opacity;
    gchar		   *metacity_button_layout;
} GWDSettingsImplPrivate;

gboolean
gwd_settings_shadow_property_changed (GWDSettingsWritable *settings,
				      gdouble     active_shadow_radius,
				      gdouble     active_shadow_opacity,
				      gdouble     active_shadow_offset_x,
				      gdouble     active_shadow_offset_y,
				      const gchar *active_shadow_color,
				      gdouble     inactive_shadow_radius,
				      gdouble     inactive_shadow_opacity,
				      gdouble     inactive_shadow_offset_x,
				      gdouble     inactive_shadow_offset_y,
				      const gchar *inactive_shadow_color)
{
    GWDSettingsImpl *settings_impl = GWD_SETTINGS_IMPL (settings);
    GWDSettingsImplPrivate *priv = GET_PRIVATE (settings_impl);

    priv->active_shadow.shadow_radius = active_shadow_radius;
    priv->active_shadow.shadow_opacity = active_shadow_opacity;
    priv->active_shadow.shadow_offset_x = active_shadow_offset_x;
    priv->active_shadow.shadow_offset_y = active_shadow_offset_y;
    memset (priv->active_shadow.shadow_color, 0, sizeof (unsigned short) * 3);
    priv->inactive_shadow.shadow_radius = inactive_shadow_radius;
    priv->inactive_shadow.shadow_opacity = inactive_shadow_opacity;
    priv->inactive_shadow.shadow_offset_x = inactive_shadow_offset_x;
    priv->inactive_shadow.shadow_offset_y = inactive_shadow_offset_y;
    memset (priv->inactive_shadow.shadow_color, 0, sizeof (unsigned short) * 3);
    return FALSE;
}

gboolean
gwd_settings_use_tooltips_changed (GWDSettingsWritable *settings,
				   gboolean    use_tooltips)
{
    return FALSE;
}

gboolean
gwd_settings_draggable_border_width_changed (GWDSettingsWritable *settings,
					     gint	 draggable_border_width)
{
    return FALSE;
}

gboolean
gwd_settings_attach_modal_dialogs_changed (GWDSettingsWritable *settings,
					   gboolean    attach_modal_dialogs)
{
    return FALSE;
}

gboolean
gwd_settings_blur_changed (GWDSettingsWritable *settings,
				    const gchar *blur_type)

{
    return FALSE;
}

gboolean
gwd_settings_metacity_theme_changed (GWDSettingsWritable *settings,
				     gboolean	 use_metacity_theme,
				     const gchar *metacity_theme)
{
    return FALSE;
}

gboolean
gwd_settings_opacity_changed (GWDSettingsWritable *settings,
				       gdouble inactive_opacity,
				       gdouble active_opacity,
				       gboolean inactive_shade_opacity,
				       gboolean active_shade_opacity)
{
    return FALSE;
}

gboolean
gwd_settings_button_layout_changed (GWDSettingsWritable *settings,
					     const gchar *button_layout)
{
    return FALSE;
}

static void gwd_settings_writable_interface_init (GWDSettingsWritableInterface *interface)
{
    interface->shadow_property_changed = gwd_settings_shadow_property_changed;
    interface->use_tooltips_changed = gwd_settings_use_tooltips_changed;
    interface->draggable_border_width_changed = gwd_settings_draggable_border_width_changed;
    interface->attach_modal_dialogs_changed = gwd_settings_attach_modal_dialogs_changed;
    interface->blur_changed = gwd_settings_blur_changed;
    interface->metacity_theme_changed = gwd_settings_metacity_theme_changed;
    interface->opacity_changed = gwd_settings_opacity_changed;
    interface->button_layout_changed = gwd_settings_button_layout_changed;
}

static void gwd_settings_interface_init (GWDSettingsInterface *interface)
{
}

static void gwd_settings_dispose (GObject *object)
{
    G_OBJECT_CLASS (gwd_settings_impl_parent_class)->dispose (object);
}

static void gwd_settings_finalize (GObject *object)
{
    GWDSettingsImplPrivate *priv = GET_PRIVATE (object);
    G_OBJECT_CLASS (gwd_settings_impl_parent_class)->finalize (object);

    if (priv->metacity_theme)
    {
	g_free (priv->metacity_theme);
	priv->metacity_theme = NULL;
    }

    if (priv->metacity_button_layout)
    {
	g_free (priv->metacity_button_layout);
	priv->metacity_button_layout = NULL;
    }
}

static void gwd_settings_set_property (GObject *object,
					guint   property_id,
					const GValue  *value,
					GParamSpec *pspec)
{
}

static void gwd_settings_get_property (GObject *object,
					guint   property_id,
					GValue  *value,
					GParamSpec *pspec)
{
    GWDSettingsImplPrivate *priv = GET_PRIVATE (object);

    switch (property_id)
    {
	case GWD_SETTINGS_IMPL_PROPERTY_ACTIVE_SHADOW:
	    g_value_set_pointer (value, &priv->active_shadow);
	    break;
	case GWD_SETTINGS_IMPL_PROPERTY_INACTIVE_SHADOW:
	    g_value_set_pointer (value, &priv->inactive_shadow);
	    break;
	case GWD_SETTINGS_IMPL_PROPERTY_USE_TOOLTIPS:
	    g_value_set_boolean (value, priv->use_tooltips);
	    break;
	case GWD_SETTINGS_IMPL_PROPERTY_DRAGGABLE_BORDER_WIDTH:
	    g_value_set_int (value, priv->draggable_border_width);
	    break;
	case GWD_SETTINGS_IMPL_PROPERTY_ATTACH_MODAL_DIALOGS:
	    g_value_set_boolean (value, priv->attach_modal_dialogs);
	    break;
	case GWD_SETTINGS_IMPL_PROPERTY_BLUR_CHANGED:
	    g_value_set_int (value, priv->blur_type);
	    break;
	case GWD_SETTINGS_IMPL_PROPERTY_METACITY_THEME:
	    g_value_set_string (value, priv->metacity_theme);
	    break;
	case GWD_SETTINGS_IMPL_PROPERTY_ACTIVE_OPACITY:
	    g_value_set_double (value, priv->metacity_active_opacity);
	    break;
	case GWD_SETTINGS_IMPL_PROPERTY_INACTIVE_OPACITY:
	    g_value_set_double (value, priv->metacity_inactive_opacity);
	    break;
	case GWD_SETTINGS_IMPL_PROPERTY_ACTIVE_SHADE_OPACITY:
	    g_value_set_boolean (value, priv->metacity_active_shade_opacity);
	    break;
	case GWD_SETTINGS_IMPL_PROPERTY_INACTIVE_SHADE_OPACITY:
	    g_value_set_boolean (value, priv->metacity_inactive_shade_opacity);
	    break;
	case GWD_SETTINGS_IMPL_PROPERTY_BUTTON_LAYOUT:
	    g_value_set_string (value, priv->metacity_button_layout);
	    break;
	default:
	    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	    break;
    }
}

static void gwd_settings_impl_class_init (GWDSettingsImplClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (klass, sizeof (GWDSettingsImplPrivate));

    object_class->dispose = gwd_settings_dispose;
    object_class->finalize = gwd_settings_finalize;
    object_class->get_property = gwd_settings_get_property;
    object_class->set_property = gwd_settings_set_property;

    g_object_class_override_property (object_class,
				      GWD_SETTINGS_IMPL_PROPERTY_ACTIVE_SHADOW,
				      "active-shadow");
    g_object_class_override_property (object_class,
				      GWD_SETTINGS_IMPL_PROPERTY_INACTIVE_SHADOW,
				      "inactive-shadow");
    g_object_class_override_property (object_class,
				      GWD_SETTINGS_IMPL_PROPERTY_USE_TOOLTIPS,
				      "use-tooltips");
    g_object_class_override_property (object_class,
				      GWD_SETTINGS_IMPL_PROPERTY_DRAGGABLE_BORDER_WIDTH,
				      "draggable-border-width");
    g_object_class_override_property (object_class,
				      GWD_SETTINGS_IMPL_PROPERTY_ATTACH_MODAL_DIALOGS,
				      "attach-modal-dialogs");
    g_object_class_override_property (object_class,
				      GWD_SETTINGS_IMPL_PROPERTY_BLUR_CHANGED,
				      "blur");
    g_object_class_override_property (object_class,
				      GWD_SETTINGS_IMPL_PROPERTY_METACITY_THEME,
				      "metacity-theme");
    g_object_class_override_property (object_class,
				      GWD_SETTINGS_IMPL_PROPERTY_ACTIVE_OPACITY,
				      "metacity-active-opacity");
    g_object_class_override_property (object_class,
				      GWD_SETTINGS_IMPL_PROPERTY_INACTIVE_OPACITY,
				      "metacity-inactive-opacity");
    g_object_class_override_property (object_class,
				      GWD_SETTINGS_IMPL_PROPERTY_ACTIVE_SHADE_OPACITY,
				      "metacity-active-shade-opacity");
    g_object_class_override_property (object_class,
				      GWD_SETTINGS_IMPL_PROPERTY_INACTIVE_SHADE_OPACITY,
				      "metacity-inactive-shade-opacity");
    g_object_class_override_property (object_class,
				      GWD_SETTINGS_IMPL_PROPERTY_BUTTON_LAYOUT,
				      "metacity-button-layout");;
}

static void gwd_settings_impl_init (GWDSettingsImpl *self)
{
}

GWDSettings *
gwd_settings_impl_new ()
{
    GWDSettingsImpl *settings = GWD_SETTINGS_IMPL (g_object_newv (GWD_TYPE_SETTINGS_IMPL, 0, NULL));

    return GWD_SETTINGS_INTERFACE (settings);
}





