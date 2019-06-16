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

#include <glm/glm.hpp>

#include <distances.h>
#include <util.h>
#include <render_util/physics.h>

namespace
{
  using util::PI;

  constexpr auto planet_radius = render_util::physics::EARTH_RADIUS;

  const int curvature_map_width = 2048;
  const int curvature_map_height = 2048;

  const int curvature_map_size = curvature_map_width * curvature_map_height;

  const int curvature_map_size_bytes = curvature_map_size * sizeof(float) * 2;

  const long double planet_circumference = (long double)2.0 * PI * (long double)planet_radius;

  const float curvature_map_max_distance = planet_circumference / 4.0;
}
