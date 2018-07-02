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

#include <engine/texunits.h>
#include <util.h>

#include <iostream>
#include <cassert>

using namespace std;

namespace
{
  #define DEFINE_TEXUNIT(name) util::makeLowercase(#name),
  string texunit_names[] =
  {
    #include <engine/texunits.priv>
  };
  #undef DEFINE_TEXUNIT

  static_assert(sizeof(texunit_names) / sizeof(string) == engine::TEXUNIT_NUM);

} // namespace


namespace engine
{

  const string &getTexUnitName(int texunit)
  {
    assert(texunit < TEXUNIT_NUM);
    return texunit_names[texunit];
  }

  int getTexUnitNumber(const std::string &texunit_name)
  {
    for (size_t i = 0; i < TEXUNIT_NUM; i++)
    {
      if (texunit_name == texunit_names[i])
        return i;
    }
    cout<<"no such texunit: "<<texunit_name<<endl;
    return -1;
  }

}
