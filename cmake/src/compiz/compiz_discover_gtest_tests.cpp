#include <map>
#include <vector>
#include <string>
#include <istream>
#include <ostream>
#include <fstream>
#include <iterator>
#include <iostream>
#include <libgen.h>

int main (int argc, char **argv)
{
    std::cin >> std::noskipws;

    if (argc < 2)
    {
	std::cout << "Usage: PATH_TO_TEST_BINARY --gtest_list_tests | ./build_test_cases PATH_TO_TEST_BINARY";
	return 1;
    }

    std::map<std::string, std::vector<std::string> > testCases;
    std::string line;
    std::string currentTestCase;

    while (std::getline (std::cin, line))
    {
	/* Is test case */
	if (line.find ("  ") == 0)
	    testCases[currentTestCase].push_back (currentTestCase + line.substr (2));
	else
	    currentTestCase = line;

    }

    std::ofstream testfilecmake;
    char *base = basename (argv[1]);
    std::string   gtestName (base);

    testfilecmake.open (std::string (gtestName  + "_test.cmake").c_str (), std::ios::out | std::ios::trunc);

    if (testfilecmake.is_open ())
    {
	for (std::map <std::string, std::vector<std::string> >::iterator it = testCases.begin ();
	     it != testCases.end (); it++)
	{
	    for (std::vector <std::string>::iterator jt = it->second.begin ();
		 jt != it->second.end (); jt++)
	    {
		if (testfilecmake.good ())
		{
		    std::string addTest ("ADD_TEST (");
		    std::string testExec (" \"" + std::string (argv[1]) + "\"");
		    std::string gTestFilter ("\"--gtest_filter=\"");
		    std::string filterBegin ("\"");
		    std::string filterEnd ("\")");

		    testfilecmake << addTest << *jt << testExec << gTestFilter << filterBegin << *jt << filterEnd << std::endl;
		}
	    }
	}

	testfilecmake.close ();
    }

    std::ifstream CTestTestfile ("CTestTestfile.cmake", std::ifstream::in);
    bool needsInclude = true;
    line.clear ();

    std::string includeLine = std::string ("INCLUDE (") +
			      gtestName  +
			      std::string ("_test.cmake)");

    if (CTestTestfile.is_open ())
    {
	while (CTestTestfile.good ())
	{
	    std::getline (CTestTestfile, line);

	    if (line == includeLine)
		needsInclude = false;
	}

	CTestTestfile.close ();
    }

    if (needsInclude)
    {
	std::ofstream CTestTestfileW ("CTestTestfile.cmake", std::ofstream::app | std::ofstream::out);

	if (CTestTestfileW.is_open ())
	{
	    CTestTestfileW << includeLine << std::endl;
	    CTestTestfileW.close ();
	}
    }

    return 0;
}
