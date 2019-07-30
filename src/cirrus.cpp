#include "spheres.h"

#include <render_util/render_util.h>
#include <render_util/cirrus_clouds.h>
#include <render_util/texture_manager.h>
#include <render_util/shader_util.h>
#include <render_util/texunits.h>
#include <render_util/texture_util.h>
#include <render_util/image_loader.h>
#include <render_util/geometry.h>
#include <render_util/globals.h>

#include <iostream>
#include <vector>
#include <map>
#include <glm/glm.hpp>

#include <render_util/gl_binding/gl_functions.h>

using namespace spheres;
using namespace render_util::gl_binding;
using std::cout;
using std::endl;

namespace
{


using Lookup=std::map<std::pair<Index, Index>, Index>;


struct NormalsCreator
{
  const IndexedMesh &mesh;

  NormalsCreator(const IndexedMesh &mesh) : mesh(mesh) {}

  glm::vec3 calcTriangleNormal(const Triangle &triangle)
  {
    glm::vec3 vertices[3];
    for (unsigned int i = 0; i < 3; i++)
      vertices[i] = mesh.vertices.at(triangle.vertex[i]);

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
        auto vertex = triangle.vertex[i_vertex];
//         cout
        normals.at(vertex) = glm::normalize(normals.at(vertex) + normal);
//         normals.at(vertex) = glm::vec3(0);
      }
    }

    return normals;
  }
};


IndexedMesh make_uv_dome()
{
  return spheres::generateUVDome(100, 100, 0.04 * util::PI);
}


class VertexArrayObject
{
  struct Vertex
  {
    float x = 0;
    float y = 0;
    float z = 0;
  };

  GLuint vao_id = 0;
  GLuint vertex_buffer_id = 0;
  GLuint normal_buffer_id = 0;

public:
  VertexArrayObject(const IndexedMesh &mesh)
  {
    gl::GenBuffers(1, &vertex_buffer_id);
    assert(vertex_buffer_id > 0);
    gl::GenBuffers(1, &normal_buffer_id);
    assert(normal_buffer_id > 0);
    gl::GenVertexArrays(1, &vao_id);
    assert(vao_id > 0);

    gl::BindVertexArray(vao_id);


    std::vector<Vertex> vertex_data;
    for (auto v : mesh.vertices)
    {
      vertex_data.push_back({v.x, v.y, v.z});
    }

    NormalsCreator normals_creator(mesh);
    cout<<"create normals .."<<endl;
    auto normals = normals_creator.createNormals();
    cout<<"create normals .. done"<<endl;

    std::vector<render_util::Float3> normal_data;
    for (auto &n : normals)
    {
      normal_data.push_back({n.x, n.y, n.z});
    }


    gl::BindBuffer(GL_ARRAY_BUFFER, vertex_buffer_id);
    gl::BufferData(GL_ARRAY_BUFFER,
                  vertex_data.size() * sizeof(Vertex),
                  vertex_data.data(), GL_STATIC_DRAW);
    gl::VertexPointer(3, GL_FLOAT, 0, 0);
    gl::EnableClientState(GL_VERTEX_ARRAY);
    gl::BindBuffer(GL_ARRAY_BUFFER, 0);

    gl::BindBuffer(GL_ARRAY_BUFFER, normal_buffer_id);
    gl::BufferData(GL_ARRAY_BUFFER,
                  normal_data.size() * sizeof(render_util::Float3),
                  normal_data.data(), GL_STATIC_DRAW);
    gl::NormalPointer(GL_FLOAT, 0, 0);
    gl::EnableClientState(GL_NORMAL_ARRAY);
    gl::BindBuffer(GL_ARRAY_BUFFER, 0);

    gl::BindVertexArray(0);

    FORCE_CHECK_GL_ERROR();
  }

  void bind()
  {
    gl::BindVertexArray(vao_id);
    CHECK_GL_ERROR();
  }

  void unbind()
  {
    gl::BindVertexArray(0);
    CHECK_GL_ERROR();
  }
};


}


namespace render_util
{


struct CirrusClouds::Impl
{
//   IndexedMesh mesh = make_icosphere(8);
  IndexedMesh mesh = make_uv_dome();
  VertexArrayObject vao = VertexArrayObject(mesh);

  render_util::TexturePtr texture;
  render_util::ShaderProgramPtr program;
};


CirrusClouds::CirrusClouds(TextureManager &txmgr,
                           const ShaderSearchPath &shader_search_path,
                           const ShaderParameters &shader_params)
  : impl(std::make_unique<Impl>())
{
  //HACK
  ShaderSearchPath path = shader_search_path;
//   path.push_back("il2ge/shaders");

  impl->program = render_util::createShaderProgram("cirrus", txmgr, path, {}, shader_params);
//   impl->program->setUniformi("sampler_generic_noise",
//                              txmgr.getTexUnitNum(TEXUNIT_GENERIC_NOISE));
  impl->program->setUniformi("sampler_cirrus",
                             txmgr.getTexUnitNum(TEXUNIT_CIRRUS));

  auto texture_image =
    render_util::loadImageFromFile<render_util::ImageGreyScale>("il2ge/textures/cirrus.tga");
  assert(texture_image);
  impl->texture = render_util::createTexture<render_util::ImageGreyScale>(texture_image);
  render_util::TextureParameters<int> p;
//   p.set(GL_TEXTURE_LOD_BIAS, 1.0);
  p.apply(impl->texture);

  txmgr.bind(TEXUNIT_CIRRUS, impl->texture);

}

CirrusClouds::~CirrusClouds() {}

void CirrusClouds::draw(const Camera &camera)
{
//   gl::Disable(GL_CULL_FACE);

  if (camera.getPos().z < 7000)
    gl::FrontFace(GL_CW);
  else
    gl::FrontFace(GL_CCW);

  impl->vao.bind();

  auto program = getCurrentGLContext()->getCurrentProgram();

  program->setUniform("cirrus_height", 7000.f);
  program->setUniform("cirrus_layer_thickness", 100.f);

  if (camera.getPos().z < 7000)
  {
    gl::FrontFace(GL_CW);
    gl::DrawElements(GL_TRIANGLES, impl->mesh.triangles.size() * 3,
                    GL_UNSIGNED_INT, impl->mesh.triangles.data());
  }
  else
  {
    gl::FrontFace(GL_CW);
    gl::DrawElements(GL_TRIANGLES, impl->mesh.triangles.size() * 3,
                    GL_UNSIGNED_INT, impl->mesh.triangles.data());

    gl::FrontFace(GL_CCW);
    gl::DrawElements(GL_TRIANGLES, impl->mesh.triangles.size() * 3,
                    GL_UNSIGNED_INT, impl->mesh.triangles.data());
  }

  impl->vao.unbind();

  gl::FrontFace(GL_CCW);

  gl::Enable(GL_DEPTH_TEST);
  gl::Enable(GL_CULL_FACE);
}


ShaderProgramPtr CirrusClouds::getProgram()
{
  return impl->program;
}


}
