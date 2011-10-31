#include "test-pluginclasshandler.h"

PluginClassStorage::Indices globalPluginClassIndices (0);
unsigned int		    pluginClassHandlerIndex = 0;
bool			    debugOutput;
char			    *programName;

namespace
{
    static CompizPCHTest *gTest;
};

Global::Global ()
{
}

unsigned int
Base::allocPluginClassIndex ()
{
    unsigned int i = PluginClassStorage::allocatePluginClassIndex (globalPluginClassIndices);

    foreach (Base *b, gTest->global->bases)
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

    foreach (Base *b, gTest->global->bases)
    {
	if (globalPluginClassIndices.size () != b->pluginClasses.size ())
	    b->pluginClasses.resize (globalPluginClassIndices.size ());
    }
}

Base::Base () :
    PluginClassStorage (globalPluginClassIndices)
{
    gTest->global->bases.push_back (this);
}

Base::~Base ()
{
    gTest->global->bases.remove (this);
}

Plugin::Plugin (Base *base) :
    b (base)
{
}

Plugin::~Plugin ()
{
}

CompizPCHTest::CompizPCHTest (Global *g) :
    global (g)
{
    ValueHolder::SetDefault (static_cast<ValueHolder *> (global));
}

CompizPCHTest::~CompizPCHTest ()
{
    delete global;
}

int
main (int argc, char **argv)
{
    programName = argv[0];

    gTest = static_cast <CompizPCHTest *> (get_object (new Global ()));
    gTest->run ();

    while (gTest->plugins.size ())
    {
	Plugin *p = gTest->plugins.front ();
	gTest->plugins.pop_front ();

	delete p;
    }

    while (gTest->bases.size ())
    {
	Base *b = gTest->bases.front ();
	gTest->bases.pop_front ();

	delete b;
    }

    delete gTest;
    
    return 0;
}
