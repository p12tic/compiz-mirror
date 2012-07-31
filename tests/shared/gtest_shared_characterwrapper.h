#ifndef _COMPIZ_GTEST_SHARED_CHARACTERWRAPPER
#define _COMPIZ_GTEST_SHARED_CHARACTERWRAPPER

class CharacterWrapper :
    boost::noncopyable
{
    public:

	explicit CharacterWrapper (char *c) :
	    mChar (c)
	{
	}

	~CharacterWrapper ()
	{
	    free (mChar);
	}

	operator char * ()
	{
	    return mChar;
	}

	operator const char * () const
	{
	    return mChar;
	}

    private:

	char *mChar;
};

#endif
