#include "resize_options.h"

ResizeOptions::ResizeOptions (bool init /* = true */) :
    mOptions (ResizeOptions::OptionNum),
    mNotify (ResizeOptions::OptionNum)
{
    mOptions[ResizeOptions::MaximizeVertically].value ().set (true);
}

ResizeOptions::~ResizeOptions ()
{
}

CompOption::Vector &
ResizeOptions::getOptions ()
{
    return mOptions;
}

bool
ResizeOptions::setOption (const CompString &name, CompOption::Value &value)
{
    return true;
}
