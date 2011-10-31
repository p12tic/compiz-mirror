#include <test-pluginclasshandler.h>

class TypenamesPlugin :
    public Plugin,
    public PluginClassHandler <TypenamesPlugin, Base>
{
    public:
	TypenamesPlugin (Base *);
};

class TypenamesPluginABI1 :
    public Plugin,
    public PluginClassHandler <TypenamesPluginABI1, Base, 1>
{
    public:
	TypenamesPluginABI1 (Base *);
};

class TypenamesPluginABI2 :
    public Plugin,
    public PluginClassHandler <TypenamesPluginABI2, Base, 2>
{
    public:
	TypenamesPluginABI2 (Base *);
};

TypenamesPlugin::TypenamesPlugin (Base *base):
    Plugin (base),
    PluginClassHandler <TypenamesPlugin, Base> (base)
{
}

TypenamesPluginABI1::TypenamesPluginABI1 (Base *base):
    Plugin (base),
    PluginClassHandler <TypenamesPluginABI1, Base, 1> (base)
{
}

TypenamesPluginABI2::TypenamesPluginABI2 (Base *base):
    Plugin (base),
    PluginClassHandler <TypenamesPluginABI2, Base, 2> (base)
{
}

CompizPCHTestTypenames::CompizPCHTestTypenames (Global *g) :
    CompizPCHTest (g)
{
}

void
CompizPCHTestTypenames::run ()
{
    std::list <Plugin *>::iterator it;

    std::cout << "-= TEST: Typenames" << std::endl;

    bases.push_back (new Base ());
    plugins.push_back (new TypenamesPlugin (bases.back ()));
    bases.push_back (new Base ());
    plugins.push_back (new TypenamesPluginABI1 (bases.back ()));
    bases.push_back (new Base ());
    plugins.push_back (new TypenamesPluginABI2 (bases.back ()));

    if (!ValueHolder::Default ()->hasValue (compPrintf ("%s_index_%lu", typeid (TypenamesPlugin).name (), 0)))
    {
	std::cout << "FAIL: ValueHolder does not have value " << compPrintf ("%s_index_%lu", typeid (TypenamesPlugin).name (), 0) << std::endl;
	exit (1);
    }

    if (!ValueHolder::Default ()->hasValue (compPrintf ("%s_index_%lu", typeid (TypenamesPluginABI1).name (), 1)))
    {
	std::cout << "FAIL: ValueHolder does not have value " << compPrintf ("%s_index_%lu", typeid (TypenamesPluginABI1).name (), 0) << std::endl;
	exit (1);
    }

    if (!ValueHolder::Default ()->hasValue (compPrintf ("%s_index_%lu", typeid (TypenamesPluginABI2).name (), 2)))
    {
	std::cout << "FAIL: ValueHolder does not have value " << compPrintf ("%s_index_%lu", typeid (TypenamesPluginABI2).name (), 2) << std::endl;
	exit (1);
    }

    std::cout << "PASS: Typenames" << std::endl;
}

CompizPCHTest *
get_object (Global *g)
{
    return new CompizPCHTestTypenames (g);
}
