/**
 *    Rendering utilities
 *    Copyright (C) 2018  Jan Lepper
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef TEXTURE_UNITS_H
#define TEXTURE_UNITS_H

#include <string>

#define DEFINE_TEXUNIT(name) TEXUNIT_##name,

namespace render_util
{

  enum { MAX_TERRAIN_TEXUNITS = 4 };

  enum TexUnitsEnum : unsigned int
  {
    #include <render_util/texunits.priv>
    TEXUNIT_NUM
  };


  enum TexUnitsExtEnum : unsigned int
  {
    EXT_TEXUNIT_START = 70,
    TEXUNIT_CUSTOM_START
  };

  static_assert((unsigned int)TEXUNIT_NUM < (unsigned int)EXT_TEXUNIT_START);

  const std::string &getTexUnitName(int texunit);
  int getTexUnitNumber(const std::string &texunit_name);

}

#undef DEFINE_TEXUNIT

#endif
