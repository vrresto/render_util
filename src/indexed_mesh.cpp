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

#include "indexed_mesh.h"


namespace render_util
{


const void *IndexedMesh::getVertexData() const
{
  return vertices.data();
}


size_t IndexedMesh::getVertexDataSize() const
{
  return vertices.size() * sizeof(Vertex);
}


const void *IndexedMesh::getIndexData() const
{
  return triangles.data();
}


size_t IndexedMesh::getIndexDataSize() const
{
  return triangles.size() * sizeof(Triangle);
}


size_t IndexedMesh::getNumIndices() const
{
  return triangles.size() * 3;
}


}
