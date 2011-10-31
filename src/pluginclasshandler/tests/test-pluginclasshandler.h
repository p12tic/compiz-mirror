#include <core/pluginclasshandler.h>
#include <core/pluginclasses.h>
#include <list>
#include <boost/foreach.hpp>

#include <iostream>

#define foreach BOOST_FOREACH

extern PluginClassStorage::Indices globalPluginClassIndices;
extern unsigned int		    pluginClassHandlerIndex;
extern bool			    debugOutput;
extern char			    *programName;

class Base;

class Global:
    public ValueHolder
{
    public:

	Global ();

	std::list <Base *> bases;
};

class Base:
    public PluginClassStorage
{
    public:

	Base ();
	virtual ~Base ();

	static unsigned int allocPluginClassIndex ();
	static void freePluginClassIndex (unsigned int index);
};

class Plugin
{
    public:

	Plugin (Base *);
	virtual ~Plugin ();

	Base *b;
};

class CompizPCHTest
{
public:

     CompizPCHTest (Global *g);
     virtual ~CompizPCHTest ();

     Global *global;
     std::list <Base *> bases;
     std::list <Plugin *> plugins;

     virtual void run () = 0;
};

class CompizPCHTestConstruct :
    public CompizPCHTest
{
    public:

	CompizPCHTestConstruct (Global *g);

	void run ();
};

class CompizPCHTestGet :
    public CompizPCHTest
{
    public:

	CompizPCHTestGet (Global *g);

	void run ();
};

class CompizPCHTestTypenames :
    public CompizPCHTest
{
    public:

	CompizPCHTestTypenames (Global *g);

	void run ();
};

class CompizPCHTestIndexes :
    public CompizPCHTest
{
    public:

	CompizPCHTestIndexes (Global *g);

	void run ();

	template <typename I>
	void printFailure (I *);

    public:
	unsigned int ePluginClassHandlerIndex;
	unsigned int eIndex;
	int 	 eRefCount;
	bool	 eInitiated;
	bool	 eFailed;
	bool	 ePcFailed;
	unsigned int ePcIndex;
	bool	 eLoadFailed;
};

CompizPCHTest *
get_object (Global *);
