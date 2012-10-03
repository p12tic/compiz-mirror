/*
 * Compiz configuration system library
 *
 * Copyright (C) 2007  Dennis Kasprzyk <onestone@opencompositing.org>
 * Copyright (C) 2007  Danny Baumann <maniac@opencompositing.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <gtest/gtest.h>
#include <gtest_shared_characterwrapper.h>

#include <ccs.h>
#include <ccs-private.h>
#include <ccs-modifier-list-inl.h>

using ::testing::WithParamInterface;

namespace
{
    const std::string firstBindingString = "<First>";
    const std::string secondBindingString = "<Second>";

    const unsigned int firstBindingMask = (1 << 0);
    const unsigned int secondBindingMask = (1 << 1);
}

TEST (CCSUtilTest, TestAddKeybindingMaskToStringInitial)
{
    char         *bindingStringChar = NULL;
    unsigned int addedBindingMask = 0;

    ccsAddKeybindingMaskToString (&bindingStringChar,
				  firstBindingMask,
				  &addedBindingMask,
				  firstBindingMask,
				  firstBindingString.c_str ());

    CharacterWrapper bindingString (bindingStringChar);

    EXPECT_EQ (firstBindingString, bindingStringChar);
    EXPECT_EQ (addedBindingMask, firstBindingMask);
}

TEST (CCSUtilTest, TestAddKeybindingMaskToStringNoDuplicates)
{
    char         *bindingStringChar = NULL;
    unsigned int addedBindingMask = 0;

    ccsAddKeybindingMaskToString (&bindingStringChar,
				  firstBindingMask,
				  &addedBindingMask,
				  firstBindingMask,
				  firstBindingString.c_str ());

    ccsAddKeybindingMaskToString (&bindingStringChar,
				  firstBindingMask,
				  &addedBindingMask,
				  firstBindingMask,
				  secondBindingString.c_str ());

    CharacterWrapper bindingString (bindingStringChar);

    EXPECT_EQ (firstBindingString, bindingStringChar);
    EXPECT_EQ (addedBindingMask, firstBindingMask);
}

TEST (CCSUtilTest, TestAddStringToKeybindingMask)
{
    unsigned int bindingMask = 0;

    ccsAddStringToKeybindingMask (&bindingMask,
				  firstBindingString.c_str (),
				  firstBindingMask,
				  firstBindingString.c_str ());

    EXPECT_EQ (bindingMask, firstBindingMask);
}

namespace
{
    class ModifierParam
    {
	public:

	    ModifierParam (const char   *modifierString,
			   unsigned int modifierMask,
			   bool         match) :
		mModifierString (modifierString),
		mModifierMask (modifierMask),
		mMatch (match)
	    {
	    }

	    ModifierParam (const ModifierParam &param) :
		mModifierString (param.mModifierString),
		mModifierMask (param.mModifierMask),
		mMatch (param.mMatch)
	    {
	    }

	    friend void swap (ModifierParam &lhs, ModifierParam &rhs)
	    {
		using std::swap;

		swap (lhs.mMatch, rhs.mMatch);
		swap (lhs.mModifierMask, rhs.mModifierMask);
		swap (lhs.mModifierString, rhs.mModifierString);
	    }

	    ModifierParam &
	    operator= (const ModifierParam &other)
	    {
		ModifierParam to (other);
		swap (*this, to);
		return *this;
	    }

	    std::string       mModifierString;
	    unsigned int      mModifierMask;
	    bool              mMatch;
    };

    ::testing::internal::ParamGenerator<ModifierParam>
    GenerateModifierParams ()
    {
	std::vector <ModifierParam> params;
	params.reserve (ccsInternalUtilNumModifiers () *
			ccsInternalUtilNumModifiers ());

	for (unsigned int i = 0; i < ccsInternalUtilNumModifiers (); ++i)
	{
	    if (modifierList[i].name == std::string ("<Primary>"))
		continue;

	    for (unsigned int j = 0; j < ccsInternalUtilNumModifiers (); ++j)
	    {
		const bool modifierMatch = modifierList[i].modifier ==
					   modifierList[j].modifier;

		params.push_back (ModifierParam (modifierList[i].name,
						 modifierList[j].modifier,
						 modifierMatch));
	    }
	}

	return ::testing::ValuesIn (params);
    }

    bool
    CheckModifierListSanity ()
    {
	return (modifierList[0].modifier !=
		modifierList[1].modifier) &&
		(std::string (modifierList[0].name) !=
		 std::string (modifierList[1].name));
    }

    const char *modifierSanityMsg = "This test requires the name and modifier " \
				    "value in modifierList[0] and " \
				    "modifierList[1] to be different " \
				    "to work correctly";
}

class CCSUtilModifiersTest :
    public ::testing::Test,
    public WithParamInterface <ModifierParam>
{
};

TEST_P (CCSUtilModifiersTest, TestModifiersToString)
{
    CharacterWrapper  modifierString (ccsModifiersToString (GetParam ().mModifierMask));
    char              *modifierStringChar = modifierString;
    /* Force "<Primary>" to test as "<Control>" as "<Primary>"
     * should never be reachable */
    const std::string expectedModifierString (GetParam ().mModifierString !=
						  std::string ("<Primary>") ?
					      GetParam ().mModifierString :
					      "<Control>");

    if (GetParam ().mMatch)
	EXPECT_EQ (expectedModifierString, modifierStringChar);
    else
	EXPECT_NE (expectedModifierString, modifierStringChar);
}

TEST_P (CCSUtilModifiersTest, TestStringToModifiers)
{
    unsigned int modifierMask (ccsStringToModifiers (GetParam ().mModifierString.c_str ()));

    if (GetParam ().mMatch)
	EXPECT_EQ (GetParam ().mModifierMask, modifierMask);
    else
	EXPECT_NE (GetParam ().mModifierMask, modifierMask);
}

INSTANTIATE_TEST_CASE_P (CCSRealModifiers, CCSUtilModifiersTest,
			 GenerateModifierParams ());

TEST (CCSUtilModifierTest, TestMultiModifierToString)
{
    ASSERT_TRUE (CheckModifierListSanity ()) << modifierSanityMsg;

    const unsigned int modifierMask           = modifierList[0].modifier |
						modifierList[1].modifier;
    const std::string  expectedModifierString = std::string (modifierList[0].name) +
						std::string (modifierList[1].name);
    CharacterWrapper   modifierString (ccsModifiersToString (modifierMask));
    const char         *modifierStringChar = modifierString;

    EXPECT_EQ (expectedModifierString, modifierStringChar);
}

TEST (CCSUtilModifierTest, TestMultiStringToModifier)
{
    ASSERT_TRUE (CheckModifierListSanity ()) << modifierSanityMsg;

    const unsigned int expectedModifierMask = modifierList[0].modifier |
					      modifierList[1].modifier;
    const std::string  modifierString = std::string (modifierList[0].name) +
					std::string (modifierList[1].name);
    unsigned int       modifierMask = (ccsStringToModifiers (modifierString.c_str ()));

    EXPECT_EQ (expectedModifierMask, modifierMask);
}
