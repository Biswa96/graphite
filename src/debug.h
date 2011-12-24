/*  GRAPHITE2 LICENSING

    Copyright 2010, SIL International
    All rights reserved.

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation; either version 2.1 of License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should also have received a copy of the GNU Lesser General Public
    License along with this library in the file named "LICENSE".
    If not, write to the Free Software Foundation, 51 Franklin Street,
    Suite 500, Boston, MA 02110-1335, USA or visit their web page on the
    internet at http://www.fsf.org/licenses/lgpl.html.

Alternatively, the contents of this file may be used under the terms of the
Mozilla Public License (http://mozilla.org/MPL) or the GNU General Public
License, as published by the Free Software Foundation, either version 2
of the License or (at your option) any later version.
*/
//	debug.h
//
//  Created on: 22 Dec 2011
//      Author: tim

#pragma once

#include <utility>
#include "json.h"
#include "Position.h"

namespace graphite2
{

class CharInfo;
class Slot;

typedef std::pair<const Slot *, const Slot *>	slots;
extern json * dbgout;

json & operator << (json & j, const Position &) throw();
json & operator << (json & j, const CharInfo &) throw();
json & operator << (json & j, const Slot &) throw();
json & operator << (json & j, const Slot *) throw();
json & operator << (json & j, const slots &) throw();

inline
json & operator << (json & j, const Position & p) throw()
{
	return j << json::array << json::flat << p.x << p.y << json::close;
}

inline
json & operator << (json & j, const Slot *b) throw()
{
	return j << slots(b,0);
}

} // namespace graphite2
