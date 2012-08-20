#include <glib-object.h>

#include <stdlib.h>
#include <stdio.h>

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
    GWD_SETTINGS_IMPL_PROPERTY_BUTTON_LAYOUT = 12,
    GWD_SETTINGS_IMPL_PROPERTY_TITLEBAR_ACTION_DOUBLE_CLICK = 13,
    GWD_SETTINGS_IMPL_PROPERTY_TITLEBAR_ACTION_MIDDLE_CLICK = 14,
    GWD_SETTINGS_IMPL_PROPERTY_TITLEBAR_ACTION_RIGHT_CLICK = 15,
    GWD_SETTINGS_IMPL_PROPERTY_MOUSE_WHEEL_ACTION = 16,
    GWD_SETTINGS_IMPL_PROPERTY_TITLEBAR_FONT = 17
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
    gint		   titlebar_double_click_action;
    gint		   titlebar_middle_click_action;
    gint		   titlebar_right_click_action;
    gint		   mouse_wheel_action;
    gchar		   *titlebar_font;
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

    decor_shadow_options_t active_shadow, inactive_shadow;

    unsigned int c[4];

    active_shadow.shadow_radius = active_shadow_radius;
    active_shadow.shadow_opacity = active_shadow_opacity;
    active_shadow.shadow_offset_x = active_shadow_offset_x;
    active_shadow.shadow_offset_y = active_shadow_offset_y;

    if (sscanf (active_shadow_color,
		"#%2x%2x%2x%2x",
		&c[0], &c[1], &c[2], &c[3]) == 4)
    {
	active_shadow.shadow_color[0] = c[0] << 8 | c[0];
	active_shadow.shadow_color[1] = c[1] << 8 | c[1];
	active_shadow.shadow_color[2] = c[2] << 8 | c[2];
    }
    else
	return FALSE;

    if (sscanf (inactive_shadow_color,
		"#%2x%2x%2x%2x",
		&c[0], &c[1], &c[2], &c[3]) == 4)
    {
	inactive_shadow.shadow_color[0] = c[0] << 8 | c[0];
	inactive_shadow.shadow_color[1] = c[1] << 8 | c[1];
	inactive_shadow.shadow_color[2] = c[2] << 8 | c[2];
    }
    else
	return FALSE;

    inactive_shadow.shadow_radius = inactive_shadow_radius;
    inactive_shadow.shadow_opacity = inactive_shadow_opacity;
    inactive_shadow.shadow_offset_x = inactive_shadow_offset_x;
    inactive_shadow.shadow_offset_y = inactive_shadow_offset_y;

    gboolean changed = FALSE;

    if (decor_shadow_options_cmp (&priv->inactive_shadow,
				  &inactive_shadow))
    {
	changed |= TRUE;
	priv->inactive_shadow = inactive_shadow;
    }

    if (decor_shadow_options_cmp (&priv->active_shadow,
				  &active_shadow))
    {
	changed |= TRUE;
	priv->active_shadow = active_shadow;
    }

    return changed;
}

gboolean
gwd_settings_use_tooltips_changed (GWDSettingsWritable *settings,
				   gboolean    use_tooltips)
{
    GWDSettingsImpl *settings_impl = GWD_SETTINGS_IMPL (settings);
    GWDSettingsImplPrivate *priv = GET_PRIVATE (settings_impl);

    if (priv->use_tooltips != use_tooltips)
    {
	priv->use_tooltips = use_tooltips;
	return TRUE;
    }

    return FALSE;
}

gboolean
gwd_settings_draggable_border_width_changed (GWDSettingsWritable *settings,
					     gint	 draggable_border_width)
{
    GWDSettingsImpl *settings_impl = GWD_SETTINGS_IMPL (settings);
    GWDSettingsImplPrivate *priv = GET_PRIVATE (settings_impl);

    if (priv->draggable_border_width != draggable_border_width)
    {
	priv->draggable_border_width = draggable_border_width;
	return TRUE;
    }
    else
	return FALSE;
}

gboolean
gwd_settings_attach_modal_dialogs_changed (GWDSettingsWritable *settings,
					   gboolean    attach_modal_dialogs)
{
    GWDSettingsImpl *settings_impl = GWD_SETTINGS_IMPL (settings);
    GWDSettingsImplPrivate *priv = GET_PRIVATE (settings_impl);

    if (priv->attach_modal_dialogs != attach_modal_dialogs)
    {
	priv->attach_modal_dialogs = attach_modal_dialogs;
	return TRUE;
    }
    else
	return FALSE;
}

gboolean
gwd_settings_blur_changed (GWDSettingsWritable *settings,
			   const gchar *type)

{
    GWDSettingsImpl *settings_impl = GWD_SETTINGS_IMPL (settings);
    GWDSettingsImplPrivate *priv = GET_PRIVATE (settings_impl);
    gint new_type = -1;

    if (strcmp (type, "titlebar") == 0)
	new_type = BLUR_TYPE_TITLEBAR;
    else if (strcmp (type, "all") == 0)
	new_type = BLUR_TYPE_ALL;
    else if (strcmp (type, "none") == 0)
	new_type = BLUR_TYPE_NONE;

    if (new_type == -1)
	return FALSE;

    if (priv->blur_type != new_type)
    {
	priv->blur_type = new_type;
	return TRUE;
    }
    else
	return FALSE;
}

static void
free_and_set_metacity_theme (GWDSettingsWritable *settings,
			     const gchar	 *metacity_theme)
{
    GWDSettingsImpl *settings_impl = GWD_SETTINGS_IMPL (settings);
    GWDSettingsImplPrivate *priv = GET_PRIVATE (settings_impl);

    if (priv->metacity_theme)
	g_free (priv->metacity_theme);

    priv->metacity_theme = g_strdup (metacity_theme);
}

gboolean
gwd_settings_metacity_theme_changed (GWDSettingsWritable *settings,
				     gboolean	 use_metacity_theme,
				     const gchar *metacity_theme)
{
    GWDSettingsImpl *settings_impl = GWD_SETTINGS_IMPL (settings);
    GWDSettingsImplPrivate *priv = GET_PRIVATE (settings_impl);

    if (!metacity_theme)
	return FALSE;

    if (use_metacity_theme)
    {
	if (g_strcmp0 (metacity_theme, priv->metacity_theme) == 0)
	    return FALSE;

	free_and_set_metacity_theme (settings, metacity_theme);
    }
    else
	free_and_set_metacity_theme (settings, "");

    return TRUE;
}

gboolean
gwd_settings_opacity_changed (GWDSettingsWritable *settings,
			      gdouble inactive_opacity,
			      gdouble active_opacity,
			      gboolean inactive_shade_opacity,
			      gboolean active_shade_opacity)
{
    GWDSettingsImpl *settings_impl = GWD_SETTINGS_IMPL (settings);
    GWDSettingsImplPrivate *priv = GET_PRIVATE (settings_impl);

    if (priv->metacity_active_opacity == active_opacity &&
	priv->metacity_inactive_opacity == inactive_opacity &&
	priv->metacity_active_shade_opacity == active_shade_opacity &&
	priv->metacity_inactive_shade_opacity == inactive_shade_opacity)
	return FALSE;

    priv->metacity_active_opacity = active_opacity;
    priv->metacity_inactive_opacity = inactive_opacity;
    priv->metacity_active_shade_opacity = active_shade_opacity;
    priv->metacity_inactive_shade_opacity = inactive_shade_opacity;

    return TRUE;
}

gboolean
gwd_settings_button_layout_changed (GWDSettingsWritable *settings,
				    const gchar *button_layout)
{
    GWDSettingsImpl *settings_impl = GWD_SETTINGS_IMPL (settings);
    GWDSettingsImplPrivate *priv = GET_PRIVATE (settings_impl);

    if (!button_layout)
	return FALSE;

    if (g_strcmp0 (priv->metacity_button_layout, button_layout) == 0)
	return FALSE;

    if (priv->metacity_button_layout)
	g_free (priv->metacity_button_layout);

    priv->metacity_button_layout = g_strdup (button_layout);

    return TRUE;
}

gboolean
gwd_settings_font_changed (GWDSettingsWritable *settings,
			   gboolean		titlebar_uses_system_font,
			   const gchar		*titlebar_font)
{
    GWDSettingsImpl *settings_impl = GWD_SETTINGS_IMPL (settings);
    GWDSettingsImplPrivate *priv = GET_PRIVATE (settings_impl);

    if (!titlebar_font)
	return FALSE;

    const gchar *no_font = NULL;
    const gchar *use_font = NULL;

    if (titlebar_uses_system_font)
	use_font = no_font;
    else
	use_font = titlebar_font;

    if (g_strcmp0 (priv->titlebar_font, use_font) == 0)
	return FALSE;

    if (priv->titlebar_font)
    {
	g_free (priv->titlebar_font);
	priv->titlebar_font = NULL;
    }

    priv->titlebar_font = use_font ? g_strdup (use_font) : NULL;

    return TRUE;
}

static gboolean
get_click_action_value (const gchar *action,
			gint	    *action_value,
			gint	    default_value)
{
    if (!action_value)
	return FALSE;

    *action_value = -1;

    if (strcmp (action, "toggle_shade") == 0)
	*action_value = CLICK_ACTION_SHADE;
    else if (strcmp (action, "toggle_maximize") == 0)
	*action_value = CLICK_ACTION_MAXIMIZE;
    else if (strcmp (action, "minimize") == 0)
	*action_value = CLICK_ACTION_MINIMIZE;
    else if (strcmp (action, "raise") == 0)
	*action_value = CLICK_ACTION_RAISE;
    else if (strcmp (action, "lower") == 0)
	*action_value = CLICK_ACTION_LOWER;
    else if (strcmp (action, "menu") == 0)
	*action_value = CLICK_ACTION_MENU;
    else if (strcmp (action, "none") == 0)
	*action_value = CLICK_ACTION_NONE;

    if (*action_value == -1)
    {
	*action_value = default_value;
	return FALSE;
    }

    return TRUE;
}

static gboolean
get_wheel_action_value (const gchar *action,
			gint	    *action_value,
			gint	    default_value)
{
    if (!action_value)
	return FALSE;

    *action_value = -1;

    if (strcmp (action, "shade") == 0)
	*action_value = WHEEL_ACTION_SHADE;
    else if (strcmp (action, "none") == 0)
	*action_value = WHEEL_ACTION_NONE;

    if (*action_value == -1)
    {
	*action_value = default_value;
	return FALSE;
    }

    return TRUE;
}

gboolean
gwd_settings_actions_changed (GWDSettingsWritable *settings,
			      const gchar	   *action_double_click_titlebar,
			      const gchar	   *action_middle_click_titlebar,
			      const gchar	   *action_right_click_titlebar,
			      const gchar	   *mouse_wheel_action)
{
    GWDSettingsImpl *settings_impl = GWD_SETTINGS_IMPL (settings);
    GWDSettingsImplPrivate *priv = GET_PRIVATE (settings_impl);

    gboolean ret = FALSE;

    ret |= get_click_action_value (action_double_click_titlebar,
				   &priv->titlebar_double_click_action,
				   DOUBLE_CLICK_ACTION_DEFAULT);
    ret |= get_click_action_value (action_middle_click_titlebar,
				   &priv->titlebar_middle_click_action,
				   MIDDLE_CLICK_ACTION_DEFAULT);
    ret |= get_click_action_value (action_right_click_titlebar,
				   &priv->titlebar_right_click_action,
				   RIGHT_CLICK_ACTION_DEFAULT);
    ret |= get_wheel_action_value (mouse_wheel_action,
				   &priv->mouse_wheel_action,
				   WHEEL_ACTION_DEFAULT);

    return ret;
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
    interface->font_changed = gwd_settings_font_changed;
    interface->titlebar_actions_changed = gwd_settings_actions_changed;
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

    if (priv->titlebar_font)
    {
	g_free (priv->titlebar_font);
	priv->titlebar_font = NULL;
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
	case GWD_SETTINGS_IMPL_PROPERTY_TITLEBAR_ACTION_DOUBLE_CLICK:
	    g_value_set_int (value, priv->titlebar_double_click_action);
	    break;
	case GWD_SETTINGS_IMPL_PROPERTY_TITLEBAR_ACTION_MIDDLE_CLICK:
	    g_value_set_int (value, priv->titlebar_middle_click_action);
	    break;
	case GWD_SETTINGS_IMPL_PROPERTY_TITLEBAR_ACTION_RIGHT_CLICK:
	    g_value_set_int (value, priv->titlebar_right_click_action);
	    break;
	case GWD_SETTINGS_IMPL_PROPERTY_MOUSE_WHEEL_ACTION:
	    g_value_set_int (value, priv->mouse_wheel_action);
	    break;
	case GWD_SETTINGS_IMPL_PROPERTY_TITLEBAR_FONT:
	    g_value_set_string (value, priv->titlebar_font);
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
				      "metacity-button-layout");
    g_object_class_override_property (object_class,
				      GWD_SETTINGS_IMPL_PROPERTY_TITLEBAR_ACTION_DOUBLE_CLICK,
				      "titlebar-double-click-action");
    g_object_class_override_property (object_class,
				      GWD_SETTINGS_IMPL_PROPERTY_TITLEBAR_ACTION_MIDDLE_CLICK,
				      "titlebar-middle-click-action");
    g_object_class_override_property (object_class,
				      GWD_SETTINGS_IMPL_PROPERTY_TITLEBAR_ACTION_RIGHT_CLICK,
				      "titlebar-right-click-action");
    g_object_class_override_property (object_class,
				      GWD_SETTINGS_IMPL_PROPERTY_MOUSE_WHEEL_ACTION,
				      "mouse-wheel-action");
    g_object_class_override_property (object_class,
				      GWD_SETTINGS_IMPL_PROPERTY_TITLEBAR_FONT,
				      "titlebar-font");
}

static void gwd_settings_impl_init (GWDSettingsImpl *self)
{
    GWDSettingsImplPrivate *priv = GET_PRIVATE (self);

    priv->use_tooltips = USE_TOOLTIPS_DEFAULT;
    priv->active_shadow.shadow_radius = ACTIVE_SHADOW_RADIUS_DEFAULT;
    priv->active_shadow.shadow_opacity = ACTIVE_SHADOW_OPACITY_DEFAULT;
    priv->active_shadow.shadow_offset_x = ACTIVE_SHADOW_OFFSET_X_DEFAULT;
    priv->active_shadow.shadow_offset_y = ACTIVE_SHADOW_OFFSET_Y_DEFAULT;
    priv->active_shadow.shadow_color[0] = 0;
    priv->active_shadow.shadow_color[1] = 0;
    priv->active_shadow.shadow_color[2] = 0;
    priv->inactive_shadow.shadow_radius = INACTIVE_SHADOW_RADIUS_DEFAULT;
    priv->inactive_shadow.shadow_opacity = INACTIVE_SHADOW_OPACITY_DEFAULT;
    priv->inactive_shadow.shadow_offset_x = INACTIVE_SHADOW_OFFSET_X_DEFAULT;
    priv->inactive_shadow.shadow_offset_y = INACTIVE_SHADOW_OFFSET_Y_DEFAULT;
    priv->inactive_shadow.shadow_color[0] = 0;
    priv->inactive_shadow.shadow_color[1] = 0;
    priv->inactive_shadow.shadow_color[2] = 0;
    priv->draggable_border_width  = DRAGGABLE_BORDER_WIDTH_DEFAULT;
    priv->attach_modal_dialogs = ATTACH_MODAL_DIALOGS_DEFAULT;
    priv->blur_type = BLUR_TYPE_DEFAULT;
    priv->metacity_theme = g_strdup (METACITY_THEME_DEFAULT);
    priv->metacity_active_opacity = METACITY_ACTIVE_OPACITY_DEFAULT;
    priv->metacity_inactive_opacity = METACITY_INACTIVE_OPACITY_DEFAULT;
    priv->metacity_active_shade_opacity = METACITY_ACTIVE_SHADE_OPACITY_DEFAULT;
    priv->metacity_inactive_shade_opacity = METACITY_INACTIVE_SHADE_OPACITY_DEFAULT;
    priv->metacity_button_layout = g_strdup (METACITY_BUTTON_LAYOUT_DEFAULT);
    priv->titlebar_double_click_action = DOUBLE_CLICK_ACTION_DEFAULT;
    priv->titlebar_middle_click_action = MIDDLE_CLICK_ACTION_DEFAULT;
    priv->titlebar_right_click_action = RIGHT_CLICK_ACTION_DEFAULT;
    priv->mouse_wheel_action = WHEEL_ACTION_DEFAULT;
    priv->titlebar_font = g_strdup (TITLEBAR_FONT_DEFAULT);
}

GWDSettings *
gwd_settings_impl_new ()
{
    GWDSettingsImpl *settings = GWD_SETTINGS_IMPL (g_object_newv (GWD_TYPE_SETTINGS_IMPL, 0, NULL));

    return GWD_SETTINGS_INTERFACE (settings);
}





