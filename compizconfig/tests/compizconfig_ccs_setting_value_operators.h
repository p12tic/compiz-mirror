#ifndef _COMPIZCONFIG_CCS_SETTING_VALUE_OPERATORS_H
#define _COMPIZCONFIG_CCS_SETTING_VALUE_OPERATORS_H

#include <ccs.h>

bool
operator== (const CCSSettingColorValue &lhs,
	    const CCSSettingColorValue &rhs)
{
    if (ccsIsEqualColor (lhs, rhs))
	return true;
    return false;
}

::std::ostream &
operator<< (::std::ostream &os, const CCSSettingColorValue &v)
{
    return os << "Red: " << std::hex << v.color.red << "Blue: " << std::hex << v.color.blue << "Green: " << v.color.green << "Alpha: " << v.color.alpha
       << std::dec << std::endl;
}

bool
operator== (const CCSSettingKeyValue &lhs,
	    const CCSSettingKeyValue &rhs)
{
    if (ccsIsEqualKey (lhs, rhs))
	return true;
    return false;
}

::std::ostream &
operator<< (::std::ostream &os, const CCSSettingKeyValue &v)
{
    return os << "Keysym: " << v.keysym << " KeyModMask " << std::hex << v.keyModMask << std::dec << std::endl;
}

bool
operator== (const CCSSettingButtonValue &lhs,
	    const CCSSettingButtonValue &rhs)
{
    if (ccsIsEqualButton (lhs, rhs))
	return true;
    return false;
}

::std::ostream &
operator<< (::std::ostream &os, const CCSSettingButtonValue &v)
{
    return os << "Button " << v.button << "Button Key Mask: " << std::hex << v.buttonModMask << "Edge Mask: " << v.edgeMask << std::dec << std::endl;
}

bool
operator== (const CCSString &lhs,
	    const std::string &rhs)
{
    if (rhs == lhs.value)
	return true;

    return false;
}

bool
operator== (const std::string &lhs,
	    const CCSString &rhs)
{
    return rhs == lhs;
}

bool
operator== (const std::string &rhs,
	    CCSString	      *lhs)
{
    return *lhs == rhs;
}

::std::ostream &
operator<< (::std::ostream &os, CCSString &string)
{
    os << string.value << std::endl;
    return os;
}

#endif
