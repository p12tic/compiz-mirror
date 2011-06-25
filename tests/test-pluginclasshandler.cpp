#include <core/pluginclasshandler.h>
#include <core/pluginclasses.h>
#include <list>
#include <boost/foreach.hpp>

#define foreach BOOST_FOREACH

PluginClassStorage::Indices globalPluginClassIndices (0);
unsigned int		    pluginClassHandlerIndex = 0;
bool			    debugOutput;
char			    *programName;

class Base;

class Global:
    public ValueHolder
{
    public:

	Global ();

	std::list <Base *> bases;
};

namespace
{
    static Global *global;
};

class Base:
    public PluginClassStorage
{
    public:

	Base ();
	~Base ();

	static unsigned int allocPluginClassIndex ();
	static void freePluginClassIndex (unsigned int index);
};

class Plugin:
    public PluginClassHandler <Plugin, Base>
{
    public:

	Plugin (Base *);
	~Plugin ();
};

Global::Global ()
{
}

unsigned int
Base::allocPluginClassIndex ()
{
    unsigned int i = PluginClassStorage::allocatePluginClassIndex (globalPluginClassIndices);

    foreach (Base *b, global->bases)
    {
	if (globalPluginClassIndices.size () != b->pluginClasses.size ())
	    b->pluginClasses.resize (globalPluginClassIndices.size ());
    }

    return i;
}

void
Base::freePluginClassIndex (unsigned int index)
{
    PluginClassStorage::freePluginClassIndex (globalPluginClassIndices, index);

    foreach (Base *b, global->bases)
    {
	if (globalPluginClassIndices.size () != b->pluginClasses.size ())
	    b->pluginClasses.resize (globalPluginClassIndices.size ());
    }
}

Base::Base () :
    PluginClassStorage (globalPluginClassIndices)
{
    global->bases.push_back (this);
}

Base::~Base ()
{
    global->bases.remove (this);
}

Plugin::Plugin (Base *base) :
    PluginClassHandler <Plugin, Base> (base)
{
}

Plugin::~Plugin ()
{
}

int
main (int argc, char **argv)
{
    programName = argv[0];

    global = new Global;

    ValueHolder::SetDefault (static_cast<ValueHolder *> (global));

    Base *b = new Base ();
    Plugin *p = new Plugin (b);

    delete p;
    delete b;
    delete global;
    
    return 0;
}
