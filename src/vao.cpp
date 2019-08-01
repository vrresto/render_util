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

#include "vao.h"

#include <render_util/geometry.h>
#include <render_util/gl_binding/gl_functions.h>

#include <glm/gtc/type_ptr.hpp>


using namespace render_util::gl_binding;
using namespace render_util;


namespace
{


struct NormalsCreator
{
  const IndexedMesh &mesh;

  NormalsCreator(const IndexedMesh &mesh) : mesh(mesh) {}

  glm::vec3 calcTriangleNormal(const IndexedMesh::Triangle &triangle)
  {
    glm::vec3 vertices[3];
    for (unsigned int i = 0; i < 3; i++)
    {
      vertices[i] = glm::make_vec3(mesh.vertices.at(triangle[i]).data());
    }

    return render_util::calcNormal(vertices);
  }

  std::vector<glm::vec3> createNormals()
  {
    std::vector<glm::vec3> normals(mesh.vertices.size());

    for (int i = 0; i < mesh.triangles.size(); i++)
    {
//       cout<<"i: "<<i<<endl;
      auto &triangle = mesh.triangles[i];
      auto normal = calcTriangleNormal(triangle);

      for (unsigned int i_vertex = 0; i_vertex < 3; i_vertex++)
      {
        auto vertex = triangle[i_vertex];
//         cout
        normals.at(vertex) = glm::normalize(normals.at(vertex) + normal);
//         normals.at(vertex) = glm::vec3(0);
      }
    }

    return normals;
  }
};


}


namespace render_util
{

VertexArrayObject::VertexArrayObject(const IndexedMesh &mesh, bool enable_normal_buffer) :
  m_num_indices(mesh.getNumIndices())
{
  gl::GenBuffers(1, &m_vertex_buffer_id);
  assert(m_vertex_buffer_id > 0);
  gl::GenBuffers(1, &m_normal_buffer_id);
  assert(m_normal_buffer_id > 0);
  gl::GenBuffers(1, &m_index_buffer_id);
  assert(m_index_buffer_id > 0);
  gl::GenVertexArrays(1, &m_vao_id);
  assert(m_vao_id > 0);

  gl::BindVertexArray(m_vao_id);

  gl::BindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer_id);
  gl::BufferData(GL_ARRAY_BUFFER, mesh.getVertexDataSize(), mesh.getVertexData(), GL_STATIC_DRAW);
  gl::VertexPointer(3, GL_FLOAT, 0, 0);
  gl::EnableClientState(GL_VERTEX_ARRAY);
  gl::BindBuffer(GL_ARRAY_BUFFER, 0);

  if (enable_normal_buffer)
  {
    NormalsCreator normals_creator(mesh);
    auto normals = normals_creator.createNormals();

    std::vector<render_util::Float3> normal_data;
    for (auto &n : normals)
    {
      normal_data.push_back({n.x, n.y, n.z});
    }

    gl::BindBuffer(GL_ARRAY_BUFFER, m_normal_buffer_id);
    gl::BufferData(GL_ARRAY_BUFFER,
                   normal_data.size() * sizeof(render_util::Float3),
                   normal_data.data(), GL_STATIC_DRAW);
    gl::NormalPointer(GL_FLOAT, 0, 0);
    gl::EnableClientState(GL_NORMAL_ARRAY);
    gl::BindBuffer(GL_ARRAY_BUFFER, 0);
  }

  gl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_index_buffer_id);
  gl::BufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.getIndexDataSize(), mesh.getIndexData(), GL_STATIC_DRAW);
  gl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  CHECK_GL_ERROR();

  gl::BindVertexArray(0);

  FORCE_CHECK_GL_ERROR();
}


VertexArrayObject::~VertexArrayObject()
{
  gl::DeleteVertexArrays(1, &m_vao_id);
  gl::DeleteBuffers(1, &m_vertex_buffer_id);
  gl::DeleteBuffers(1, &m_index_buffer_id);
  if (m_normal_buffer_id)
    gl::DeleteBuffers(1, &m_normal_buffer_id);
}


VertexArrayObjectBinding::VertexArrayObjectBinding(VertexArrayObject &vao)
{
  gl::BindVertexArray(vao.getID());
  CHECK_GL_ERROR();
}


VertexArrayObjectBinding::~VertexArrayObjectBinding()
{
  gl::BindVertexArray(0);
  CHECK_GL_ERROR();
}


IndexBufferBinding::IndexBufferBinding(VertexArrayObject &vao)
{
  gl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, vao.getIndexBufferID());
  CHECK_GL_ERROR();
}


IndexBufferBinding::~IndexBufferBinding()
{
  gl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  CHECK_GL_ERROR();
}


}
