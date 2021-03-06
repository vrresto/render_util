/**
 *    Rendering utilities
 *    Copyright (C) 2019 Jan Lepper
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


#include <render_util/atmosphere.h>
#include "atmosphere_precomputed.h"

namespace render_util
{


std::unique_ptr<Atmosphere> createAtmosphere(Atmosphere::Type type,
                                             render_util::TextureManager &tex_mgr,
                                             std::string shader_dir,
                                             const AtmosphereCreationParameters &params)
{
  switch (type)
  {
    case Atmosphere::PRECOMPUTED:
      return std::make_unique<AtmospherePrecomputed>(tex_mgr, shader_dir, params);
    default:
      return std::make_unique<Atmosphere>();
  }
}


}
