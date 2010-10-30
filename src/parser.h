/*
 * Compiz fragment program parser
 *
 * parser.h
 *
 * This should be usable on almost any plugin that wishes to parse fragment
 * program files separately, maybe it should become a separate plugin?
 *
 * Author : Guillaume Seguin
 * Email : guillaume@segu.in
 *
 * Copyright (c) 2007 Guillaume Seguin <guillaume@segu.in>
 *
 * Basic C++ port of this by:
 * Copyright (c) 2009 Sam Spilsbury <smspillaz@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "colorfilter.h"

class FragmentParser
{
    public:
	enum
	{
	    NoOp,
	    DataOp,
	    StoreDataOp,
	    OffsetDataOp,
	    BlendDataOp,
	    FetchOp,
	    ColorOp,
	    LoadOp,
	    TempOp,
	    ParamOp,
	    AttribOp,
	} OpType;

	class FragmentOffset
	{
	    public:

		CompString name;
		CompString offset;
	};

	std::list <FragmentOffset *> offsets;

	CompString
	base_name (CompString str);

	CompString
	ltrim (CompString string);

	CompString
	programCleanName (CompString name);

	CompString
	programReadSource (CompString fname);

	CompString
	getFirstArgument (char **source);

	FragmentOffset *
	programAddOffsetFromAddOp (char *source);

	CompString
	programFindOffset (std::list<FragmentOffset *>::iterator it,
			   const CompString &name);

	void
	programFreeOffset ();

	void
	programParseSource (GLFragment::FunctionData *data,
			    int target, char *source);

	GLFragment::FunctionId
	buildFragmentProgram (CompString &, CompString &, int target);

	GLFragment::FunctionId
	loadFragmentProgram (CompString &file,
			     CompString &name,
			     int target);


};
