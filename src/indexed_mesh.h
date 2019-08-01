/**
 *    Rendering utilities
 *    Copyright (C) 2019  Jan Lepper
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

#ifndef RENDER_UTIL_INDEXED_MESH_H
#define RENDER_UTIL_INDEXED_MESH_H

#include <vector>
#include <array>

#include <glm/glm.hpp>

namespace render_util
{


struct IndexedMesh
{
  using Index = uint32_t;

  using Triangle = std::array<Index,3>;
  using Vertex = std::array<float,3>;

  using TriangleList=std::vector<Triangle>;
  using VertexList=std::vector<Vertex>;

  VertexList vertices;
  TriangleList triangles;

  void addVertex(float x, float y, float z)
  {
    vertices.push_back({x, y, z});
  }

  void addTriangle(uint32_t a, uint32_t b, uint32_t c)
  {
    triangles.push_back({a, b, c});
  }

  void addQuad(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
  {
    addTriangle(a, c, b);
    addTriangle(a, d, c);
  }

  size_t getNumIndices() const;

  const void *getVertexData() const;
  size_t getVertexDataSize() const;

  const void *getIndexData() const;
  size_t getIndexDataSize() const;
};


}

#endif
