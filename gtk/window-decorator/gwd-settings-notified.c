#include <glib-object.h>

#include "gwd-settings-notified-interface.h"
#include "gwd-settings-notified.h"

#include "gtk-window-decorator.h"

#define GWD_SETTINGS_NOTIFIED(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GWD_TYPE_SETTINGS_NOTIFIED, GWDSettingsNotified));
#define GWD_SETTINGS_NOTIFIED_CLASS(obj) (G_TYPE_CHECK_CLASS_CAST ((obj), GWD_TYPE_SETTINGS_NOTIFIED, GWDSettingsNotifiedClass));
#define GWD_IS_MOCK_SETTINGS(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GWD_TYPE_SETTINGS_NOTIFIED));
#define GWD_IS_MOCK_SETTINGS_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((obj), GWD_TYPE_SETTINGS_NOTIFIED));
#define GWD_SETTINGS_NOTIFIED_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GWD_TYPE_SETTINGS_NOTIFIED, GWDSettingsNotifiedClass));

typedef struct _GWDSettingsNotified
{
    GObject parent;
} GWDSettingsNotified;

typedef struct _GWDSettingsNotifiedClass
{
    GObjectClass parent_class;
} GWDSettingsNotifiedClass;

static void gwd_settings_notified_interface_init (GWDSettingsNotifiedInterface *interface);

G_DEFINE_TYPE_WITH_CODE (GWDSettingsNotified, gwd_settings_notified, G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (GWD_TYPE_SETTINGS_NOTIFIED_INTERFACE,
						gwd_settings_notified_interface_init))

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GWD_TYPE_SETTINGS_NOTIFIED, GWDSettingsNotifiedPrivate))

typedef struct _GWDSettingsNotifiedPrivate
{
} GWDSettingsNotifiedPrivate;

static void gwd_settings_notified_interface_init (GWDSettingsNotifiedInterface *interface)
{
}

static void gwd_settings_notified_dispose (GObject *object)
{
}

static void gwd_settings_notified_finalize (GObject *object)
{
    G_OBJECT_CLASS (gwd_settings_notified_parent_class)->finalize (object);
}

static void gwd_settings_notified_class_init (GWDSettingsNotifiedClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (klass, sizeof (GWDSettingsNotifiedPrivate));

    object_class->dispose = gwd_settings_notified_dispose;
    object_class->finalize = gwd_settings_notified_finalize;
}

void gwd_settings_notified_init (GWDSettingsNotified *self)
{
}

GWDSettingsNotified *
gwd_settings_notified_new ()
{
    GWDSettingsNotified *storage = GWD_SETTINGS_NOTIFIED_INTERFACE (g_object_newv (GWD_TYPE_SETTINGS_NOTIFIED, 0, NULL));

    return storage;
}
