#include <glib-object.h>

#include "gwd-settings-notified-interface.h"
#include "gwd-settings-notified.h"

#include "gtk-window-decorator.h"

#define GWD_SETTINGS_NOTIFIED(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GWD_TYPE_SETTINGS_NOTIFIED, GWDSettingsNotifiedImpl));
#define GWD_SETTINGS_NOTIFIED_CLASS(obj) (G_TYPE_CHECK_CLASS_CAST ((obj), GWD_TYPE_SETTINGS_NOTIFIED, GWDSettingsNotifiedImplClass));
#define GWD_IS_MOCK_SETTINGS(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GWD_TYPE_SETTINGS_NOTIFIED));
#define GWD_IS_MOCK_SETTINGS_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((obj), GWD_TYPE_SETTINGS_NOTIFIED));
#define GWD_SETTINGS_NOTIFIED_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GWD_TYPE_SETTINGS_NOTIFIED, GWDSettingsNotifiedImplClass));

typedef struct _GWDSettingsNotifiedImpl
{
    GObject parent;
} GWDSettingsNotifiedImpl;

typedef struct _GWDSettingsNotifiedImplClass
{
    GObjectClass parent_class;
} GWDSettingsNotifiedImplClass;

static void gwd_settings_notified_impl_interface_init (GWDSettingsNotifiedInterface *interface);

G_DEFINE_TYPE_WITH_CODE (GWDSettingsNotifiedImpl, gwd_settings_notified_impl, G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (GWD_TYPE_SETTINGS_NOTIFIED_INTERFACE,
						gwd_settings_notified_impl_interface_init))

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GWD_TYPE_SETTINGS_NOTIFIED, GWDSettingsNotifiedImplPrivate))

typedef struct _GWDSettingsNotifiedImplPrivate
{
    WnckScreen *screen;
} GWDSettingsNotifiedImplPrivate;

static gboolean
gwd_settings_notified_impl_update_decorations (GWDSettingsNotified *notified)
{
    GWDSettingsNotifiedImplPrivate *priv = GET_PRIVATE (notified);
    decorations_changed (priv->screen);
    return TRUE;
}

void
set_frame_scale (decor_frame_t *frame,
		 const gchar   *font_str)
{
    gfloat	  scale = 1.0f;

    gwd_decor_frame_ref (frame);

    if (frame->titlebar_font)
	pango_font_description_free (frame->titlebar_font);

    frame->titlebar_font = pango_font_description_from_string (font_str);

    scale = (*theme_get_title_scale) (frame);

    pango_font_description_set_size (frame->titlebar_font,
				     MAX (pango_font_description_get_size (frame->titlebar_font) * scale, 1));

    gwd_decor_frame_unref (frame);
}

void
set_frames_scales (gpointer key,
		   gpointer value,
		   gpointer user_data)
{
    decor_frame_t *frame = (decor_frame_t *) value;
    gchar	  *font_str = (gchar *) user_data;

    gwd_decor_frame_ref (frame);

    set_frame_scale (frame, font_str);

    gwd_decor_frame_unref (frame);
}

static gboolean
gwd_settings_notified_impl_update_frames (GWDSettingsNotified *notified)
{
    const gchar *titlebar_font = NULL;
    g_object_get (settings, "titlebar-font", &titlebar_font, NULL);

    gwd_frames_foreach (set_frames_scales, (gpointer) titlebar_font);
    return TRUE;
}

static gboolean
gwd_settings_notified_impl_update_metacity_theme (GWDSettingsNotified *notified)
{
#ifdef USE_METACITY
    const gchar *meta_theme = NULL;
    g_object_get (settings, "metacity-theme", &meta_theme, NULL);

    if (meta_theme)
    {
	const gchar *theme = meta_theme;

	if (theme)
	{
	    meta_theme_set_current (theme, TRUE);
	    if (!meta_theme_get_current ())
		meta_theme = NULL;
	}
    }

    if (meta_theme)
    {
	theme_draw_window_decoration	= meta_draw_window_decoration;
	theme_calc_decoration_size	= meta_calc_decoration_size;
	theme_update_border_extents	= meta_update_border_extents;
	theme_get_event_window_position = meta_get_event_window_position;
	theme_get_button_position	= meta_get_button_position;
	theme_get_title_scale	    	= meta_get_title_scale;
	theme_get_shadow		= meta_get_shadow;
    }
    else
    {
	theme_draw_window_decoration	= draw_window_decoration;
	theme_calc_decoration_size	= calc_decoration_size;
	theme_update_border_extents	= update_border_extents;
	theme_get_event_window_position = get_event_window_position;
	theme_get_button_position	= get_button_position;
	theme_get_title_scale	    	= get_title_scale;
	theme_get_shadow		= cairo_get_shadow;
    }

    return TRUE;
#else
    theme_draw_window_decoration    = draw_window_decoration;
    theme_calc_decoration_size	    = calc_decoration_size;
    theme_update_border_extents	    = update_border_extents;
    theme_get_event_window_position = get_event_window_position;
    theme_get_button_position	    = get_button_position;
    theme_get_title_scale	    = get_title_scale;
    theme_get_shadow		    = cairo_get_shadow;

    return FALSE;
#endif
}

static gboolean
gwd_settings_notified_impl_update_metacity_button_layout (GWDSettingsNotified *notified)
{
    const gchar *button_layout;
    g_object_get (settings, "metacity-button-layout", &button_layout, NULL);

    if (button_layout)
    {
	meta_update_button_layout (button_layout);

	meta_button_layout_set = TRUE;

	return TRUE;
    }

    if (meta_button_layout_set)
    {
	meta_button_layout_set = FALSE;
	return TRUE;
    }

    return FALSE;
}

static void gwd_settings_notified_impl_interface_init (GWDSettingsNotifiedInterface *interface)
{
    interface->update_decorations = gwd_settings_notified_impl_update_decorations;
    interface->update_frames = gwd_settings_notified_impl_update_frames;
    interface->update_metacity_button_layout = gwd_settings_notified_impl_update_metacity_button_layout;
    interface->update_metacity_theme = gwd_settings_notified_impl_update_metacity_theme;
}

static void gwd_settings_notified_impl_dispose (GObject *object)
{
}

static void gwd_settings_notified_impl_finalize (GObject *object)
{
    G_OBJECT_CLASS (gwd_settings_notified_impl_parent_class)->finalize (object);
}

static void gwd_settings_notified_impl_class_init (GWDSettingsNotifiedImplClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (klass, sizeof (GWDSettingsNotifiedImplPrivate));

    object_class->dispose = gwd_settings_notified_impl_dispose;
    object_class->finalize = gwd_settings_notified_impl_finalize;
}

void gwd_settings_notified_impl_init (GWDSettingsNotifiedImpl *self)
{
}

GWDSettingsNotified *
gwd_settings_notified_impl_new ()
{
    GWDSettingsNotified *storage = GWD_SETTINGS_NOTIFIED_INTERFACE (g_object_newv (GWD_TYPE_SETTINGS_NOTIFIED, 0, NULL));

    return storage;
}
