#include <test-pluginclasshandler.h>

class ConstructPlugin :
    public Plugin,
    public PluginClassHandler <ConstructPlugin, Base>
{
    public:
	ConstructPlugin (Base *);
};

ConstructPlugin::ConstructPlugin (Base *base):
    Plugin (base),
    PluginClassHandler <ConstructPlugin, Base> (base)
{
}

CompizPCHTestConstruct::CompizPCHTestConstruct (Global *g) :
    CompizPCHTest (g)
{
}

void
CompizPCHTestConstruct::run ()
{
    Plugin *p;

    std::cout << "-= TEST: Basic construction" << std::endl;

    bases.push_back (new Base ());
    plugins.push_back (static_cast <Plugin *> (new ConstructPlugin (bases.back ())));
    bases.push_back (new Base ());
    plugins.push_back (static_cast <Plugin *> (new ConstructPlugin (bases.back ())));

    if (bases.front ()->pluginClasses.size () != globalPluginClassIndices.size ())
    {
	std::cout << "FAIL: allocated number of plugin classes is not the same as the"\
		     " global number of allocated plugin classes" << std::endl;
	exit (1);
    }

    if (!ValueHolder::Default ()->hasValue (compPrintf ("%s_index_%lu", typeid (ConstructPlugin).name (), 0)))
    {
	std::cout << "FAIL: ValueHolder does not have value " << compPrintf ("%s_index_%lu", typeid (ConstructPlugin).name (), 0) << std::endl;
	exit (1);
    }

    p = ConstructPlugin::get (bases.front ());

    if (p != plugins.front ())
    {
	std::cout << "FAIL: Returned Plugin * is not plugins.front ()" << std::endl;
	exit (1);
    }

    p = ConstructPlugin::get (bases.back ());

    if (p != plugins.back ())
    {
	std::cout << "FAIL: Returned Plugin * is not plugins.back ()" << std::endl;
	exit (1);
    }

    std::cout << "PASS: Basic construction" << std::endl;
}
