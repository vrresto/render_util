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

#ifndef VIEWER_MAIN_H
#define VIEWER_MAIN_H

#include "scene.h"

#include <memory>
#include <functional>

void runApplication(std::function<std::shared_ptr<Scene>()> f_create_scene);

// template <typename T>
// void runApplication()
// {
//   runApplication( [] { return std::shared_ptr<Scene>(new T); } );
// }

#endif
