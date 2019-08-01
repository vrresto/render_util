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

#ifndef RENDER_UTIL_VAO_H
#define RENDER_UTIL_VAO_H

#include "indexed_mesh.h"

namespace render_util
{


class VertexArrayObject
{
  unsigned int m_vao_id = 0;
  unsigned int m_vertex_buffer_id = 0;
  unsigned int m_normal_buffer_id = 0;
  unsigned int m_index_buffer_id = 0;
  unsigned int m_num_indices = 0;

public:
  VertexArrayObject(const IndexedMesh &mesh, bool enable_normal_buffer);
  ~VertexArrayObject();

  unsigned int getID() { return m_vao_id; }
  unsigned int getIndexBufferID() { return m_index_buffer_id; }
  unsigned int getNumIndices() { return m_num_indices; }
};


class VertexArrayObjectBinding
{
public:
  VertexArrayObjectBinding(VertexArrayObject &vao);
  ~VertexArrayObjectBinding();
};


class IndexBufferBinding
{
public:
  IndexBufferBinding(VertexArrayObject &vao);
  ~IndexBufferBinding();
};


}

#endif
