#include <test-pluginclasshandler.h>

class GetPlugin :
    public Plugin,
    public PluginClassHandler <GetPlugin, Base>
{
    public:
	GetPlugin (Base *);
};

GetPlugin::GetPlugin (Base *base):
    Plugin (base),
    PluginClassHandler <GetPlugin, Base> (base)
{
}

CompizPCHTestGet::CompizPCHTestGet (Global *g) :
    CompizPCHTest (g)
{
}

void
CompizPCHTestGet::run ()
{
    Plugin *p;

    std::cout << "-= TEST: Object Retreival" << std::endl;

    bases.push_back (new Base ());
    plugins.push_back (new GetPlugin (bases.back ()));
    bases.push_back (new Base ());
    plugins.push_back (new GetPlugin (bases.back ()));

    p = GetPlugin::get (bases.front ());

    if (p != plugins.front ())
    {
	std::cout << "FAIL: Returned Plugin * is not plugins.front ()" << std::endl;
	exit (1);
    }

    p = GetPlugin::get (bases.back ());

    if (p != plugins.back ())
    {
	std::cout << "FAIL: Returned Plugin * is not plugins.back ()" << std::endl;
	exit (1);
    }

    /* Now create a third base and check if plugin is implicitly created */

    bases.push_back (new Base ());
    plugins.push_back (GetPlugin::get (bases.back ()));

    p = plugins.back ();

    if (p->b != bases.back ())
    {
	std::cout << "FAIL: Returned Plugin * is not the plugin for bases.back ()" << std::endl;
	exit (1);
    }

    std::cout << "PASS: Object Retreival" << std::endl;
}

CompizPCHTest *
get_object (Global *g)
{
    return new CompizPCHTestGet (g);
}
