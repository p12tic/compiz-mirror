#include <gsettings_mocks.h>

static void compizconfig_gsettings_wrap_gsettings_default_init (CCSGSettingsWrapGSettingsInterface *interface);

G_DEFINE_INTERFACE (CCSGSettingsWrapGSettings, compizconfig_gsettings_wrap_gsettings, G_TYPE_OBJECT)

static void compizconfig_gsettings_wrap_gsettings_default_init (CCSGSettingsWrapGSettingsInterface *klass)
{
    g_object_interface_install_property (klass,
					 g_param_spec_string ("name",
							      "Name",
							      "Schema Name",
							      "invalid.invalid",
							      static_cast <GParamFlags> (G_PARAM_CONSTRUCT_ONLY |
											 G_PARAM_WRITABLE |
											 G_PARAM_READABLE)));
}

CCSGSettingsWrapGSettings *
compizconfig_gsettings_wrap_gsettings_new (GType type, const gchar *name)
{
    GValue name_v = G_VALUE_INIT;

    g_value_init (&name_v, G_TYPE_STRING);

    g_value_set_string (&name_v, name);

    GParameter param[1] =
    {
	{ "name", name_v }
    };

    CCSGSettingsWrapGSettings *wrap_gsettings = COMPIZCONFIG_GSETTINGS_WRAP_GSETTINGS (g_object_newv (type, 1, param));

    g_value_unset (&name_v);

    return wrap_gsettings;
}

G_BEGIN_DECLS

#define COMPIZCONFIG_GSETTINGS_MOCK_WRAP_GSETTINGS(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), COMPIZCONFIG_GSETTINGS_TYPE_MOCK_WRAP_GSETTINGS, CCSGSettingsMockWrapGSettings))

#define COMPIZCONFIG_GSETTINGS_TYPE_MOCK_GSETTINGS_CONCRETE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), COMPIZCONFIG_GSETTINGS_TYPE_MOCK_WRAP_GSETTINGS, CCSGSettingsMockWrapGSettingsClass))

#define COMPIZCONFIG_GSETTINGS_IS_MOCK_WRAP_GSETTINGS(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), COMPIZCONFIG_GSETTINGS_TYPE_MOCK_WRAP_GSETTINGS)

#define COMPIZCONFIG_GSETTINGS_IS_MOCK_WRAP_GSETTINGS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), COMPIZCONFIG_GSETTINGS_TYPE_MOCK_WRAP_GSETTINGS))

#define COMPIZCONFIG_GSETTINGS_TYPE_MOCK_GSETTINGS_CONCRETE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), COMPIZCONFIG_GSETTINGS_TYPE_MOCK_WRAP_GSETTINGS, CCSGSettingsMockWrapGSettingsClass))

typedef struct {
  GObject parent;
} CCSGSettingsMockWrapGSettings;

typedef struct {
  GObjectClass parent_class;
} CCSGSettingsMockWrapGSettingsClass;

G_END_DECLS

static void compizconfig_gsettings_mock_wrap_gsettings_interface_init (CCSGSettingsWrapGSettingsInterface *interface);

G_DEFINE_TYPE_WITH_CODE (CCSGSettingsMockWrapGSettings, compizconfig_gsettings_mock_wrap_gsettings, G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (COMPIZCONFIG_GSETTINGS_TYPE_WRAP_GSETTINGS,
						compizconfig_gsettings_mock_wrap_gsettings_interface_init))

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), COMPIZCONFIG_GSETTINGS_TYPE_MOCK_WRAP_GSETTINGS, CCSGSettingsMockWrapGSettingsPrivate))

typedef struct _CCSGSettingsMockWrapGSettingsPrivate CCSGSettingsMockWrapGSettingsPrivate;

struct _CCSGSettingsMockWrapGSettingsPrivate
{
    gchar *name;
};

static void
compizconfig_gsettings_mock_wrap_gsettings_dispose (GObject *object)
{
    CCSGSettingsMockWrapGSettingsPrivate *priv = GET_PRIVATE (object);
    G_OBJECT_CLASS (compizconfig_gsettings_mock_wrap_gsettings_parent_class)->dispose (object);

    g_print ("disposed %s\n", priv->name);

    if (priv->name)
	g_free (priv->name);

    priv->name = NULL;
}

static void
compizconfig_gsettings_mock_wrap_gsettings_finalize (GObject *object)
{
    G_OBJECT_CLASS (compizconfig_gsettings_mock_wrap_gsettings_parent_class)->finalize (object);
}

static void
compizconfig_gsettings_mock_wrap_gsettings_interface_init (CCSGSettingsWrapGSettingsInterface *interface)
{
}

static void
compizconfig_gsettings_mock_wrap_gsettings_set_property (GObject *object,
							 guint   property_id,
							 const   GValue *value,
							 GParamSpec *pspec)
{
    CCSGSettingsMockWrapGSettingsPrivate *priv = GET_PRIVATE (object);

    switch (property_id)
    {
	case COMPIZCONFIG_GSETTINGS_WRAP_GSETTINGS_PROPERTY_SCHEMA_NAME:
	    if (priv->name)
		g_free (priv->name);

	    priv->name = g_value_dup_string (value);
	    break;
	default:
	    g_assert_not_reached ();
	    break;
    }
}

static void
compizconfig_gsettings_mock_wrap_gsettings_get_property (GObject *object,
							 guint   property_id,
							 GValue  *value,
							 GParamSpec *pspec)
{
    CCSGSettingsMockWrapGSettingsPrivate *priv = GET_PRIVATE (object);

    switch (property_id)
    {
	case COMPIZCONFIG_GSETTINGS_WRAP_GSETTINGS_PROPERTY_SCHEMA_NAME:
	    g_value_set_string (value, priv->name);
	    break;
	default:
	    g_assert_not_reached ();
	    break;
    }
}

static GObject *
compizconfig_gsettings_mock_wrap_gsettings_constructor (GType		      type,
							guint		      n_construct_properties,
							GObjectConstructParam *construction_properties)
{
    GObject *object;
    CCSGSettingsMockWrapGSettingsPrivate *priv;

    {
	object = G_OBJECT_CLASS (compizconfig_gsettings_mock_wrap_gsettings_parent_class)->constructor (type,
													n_construct_properties,
													construction_properties);
    }

    priv = GET_PRIVATE (object);

    for (guint i = 0; i < n_construct_properties; i++)
    {
	if (g_strcmp0 (construction_properties[i].pspec->name, "name") == 0)
	{
	    if (!priv->name)
		priv->name = g_value_dup_string (construction_properties[i].value);
	}
	else
	{
	    g_assert_not_reached ();
	}
    }

    return object;
}

static void
compizconfig_gsettings_mock_wrap_gsettings_class_init (CCSGSettingsMockWrapGSettingsClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (klass, sizeof (CCSGSettingsMockWrapGSettingsPrivate));

    object_class->dispose = compizconfig_gsettings_mock_wrap_gsettings_dispose;
    object_class->finalize = compizconfig_gsettings_mock_wrap_gsettings_finalize;
    object_class->set_property = compizconfig_gsettings_mock_wrap_gsettings_set_property;
    object_class->get_property = compizconfig_gsettings_mock_wrap_gsettings_get_property;
    object_class->constructor = compizconfig_gsettings_mock_wrap_gsettings_constructor;

    g_object_class_override_property (G_OBJECT_CLASS (klass),
				      COMPIZCONFIG_GSETTINGS_WRAP_GSETTINGS_PROPERTY_SCHEMA_NAME,
				      "name");
}

static void
compizconfig_gsettings_mock_wrap_gsettings_init (CCSGSettingsMockWrapGSettings *self)
{
}
