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

#include <render_util/globals.h>

#include <cassert>

using namespace render_util;


namespace
{
  Globals* g_globals = nullptr;
}


namespace render_util
{


Globals::Globals()
{
  assert(!g_globals);
  g_globals = this;
}


Globals::~Globals()
{
  assert(g_globals == this);
  g_globals = nullptr;
}


Globals *Globals::get()
{
  assert(g_globals);
  return g_globals;
}


} // namespace render_util
