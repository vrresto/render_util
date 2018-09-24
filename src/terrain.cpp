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

#include <render_util/image.h>
#include <render_util/terrain.h>
#include <render_util/elevation_map.h>
#include <render_util/gl_context.h>

#include <iostream>
#include <vector>
#include <cassert>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/gl.h>

#include <gl_wrapper/gl_functions.h>

using namespace std;
using namespace gl_wrapper::gl_functions;
using namespace render_util;


namespace render_util
{
  class Terrain : public TerrainBase
  {
    struct Private;
    Private *p = 0;

  public:
    Terrain();
    ~Terrain() override;

    const std::string &getName() override;
    void build(ElevationMap::ConstPtr map) override;
    void draw() override;

    std::vector<glm::vec3> getNormals() override;
  };

}


namespace
{

  struct Coordinate
  {
    GLfloat x;
    GLfloat y;
    GLfloat z;
  };

  typedef Coordinate Vertex;
  typedef Coordinate Normal;

  glm::vec3 calcNormal(glm::vec3 vertices[3])
  {
    using namespace glm;

    vec3 a = vertices[0] - vertices[1];
    vec3 b = vertices[0] - vertices[2];

    return normalize(cross(a,b));
  }

  struct TerrainMesh
  {
    const float grid_resolution = 200.0;

    std::vector<Vertex> vertices;
    std::vector<Normal> normals;
//     std::vector<Vertex> triangle_data;
    std::vector<GLuint> triangle_data_indexed;
    ElevationMap::ConstPtr elevation_map;

    float elevation_scale = 1.0;
    float elevation_offset = 0.0;
    
    float offset_x = 0.0;
    float offset_y = 0.0;
    
    int lod_level = 0;
    int tile_factor = 1;
    
    int width = 0;
    int height = 0;

//     TerrainMesh() {}
//     TerrainMesh(const ImageGreyScale *height_map, const Terrain::ElevationFunction *elevation_func) :
//       elevation_map(height_map, elevation_func)
//     {
//     }
    ~TerrainMesh() {}

    int numVertices()
    {
      return width * height;
    }

    int vertexDataSize()
    {
      return numVertices() * sizeof(Vertex);
    }

    GLuint getVertexIndex(int x, int y)
    {
      return y * width + x;
    }

    void createVertexData()
    {
      cout<<"TerrainMesh::createVertexData()"<<endl;
      vertices.clear();

      int divisor = pow(2, lod_level);

      width = elevation_map->getWidth() * tile_factor;
      height = elevation_map->getHeight() * tile_factor;
      
      width = width / divisor;
      height = height / divisor;

      cout<<"numVertices: "<<numVertices()<<endl;
      
      assert(vertexDataSize() > 0);
      vertices.resize(numVertices());
      
      cout<<"vertices allocated."<<endl;

      float lod_grid_resolution = grid_resolution * divisor;

      for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
          float elevation = elevation_map->get(x * divisor, y * divisor);
          elevation *= elevation_scale;
          elevation += elevation_offset;
          vertices[getVertexIndex(x, y)] =
              {
                x * lod_grid_resolution + offset_x,
                y * lod_grid_resolution + offset_y,
                elevation
              };
        }
      }
    }

    void createNormalData()
    {
      cout<<"TerrainMesh::createNormalData()"<<endl;
      normals.resize(numVertices());
//       for (Normal &n : normals)
//         n = { 0,0,1 };
//       for (int y = 0; y < height; y++)
//       {
//         for (int x = 0; x < width; x++)
//         {
//           Vertex v0 = vertices[getVertexIndex(x, y)];
//         }
//       }

    }


    glm::vec3 getVertex(GLuint index) const 
    {
      return glm::make_vec3(reinterpret_cast<const GLfloat*>((vertices.data() + index)));
    }

    Normal *getNormalPtr(GLuint index)
    {
      return normals.data() + index;
    }

    const Normal *getNormalConstPtr(GLuint index) const
    {
      return normals.data() + index;
    }

    glm::vec3 getNormal(GLuint index) const
    {
      return glm::make_vec3(reinterpret_cast<const GLfloat*>(getNormalConstPtr(index)));
    }

    void setNormal(GLuint index, glm::vec3 normal)
    {
      Normal *ptr = getNormalPtr(index);
      *ptr = *reinterpret_cast<Normal*>(glm::value_ptr(normal));
    }

    glm::vec3 calcTriangleNormal(GLuint triangle[3])
    {
      using namespace glm;

      vec3 vertices[3];
      for (unsigned int i = 0; i < 3; i++)
        vertices[i] = getVertex(triangle[i]);

      return calcNormal(vertices);
    }

    void createTriangleDataIndexed()
    {
      cout<<"TerrainMesh::createTriangleDataIndexed()"<<endl;
      triangle_data_indexed.clear();
      createVertexData();
      createNormalData();

      for (int y = 0; y < height-1; y++)
      {
        for (int x = 0; x < width-1; x++)
        {
          GLuint triangle0[3] =
          {
            getVertexIndex(x+1, y+1),
            getVertexIndex(x+0, y+0),
            getVertexIndex(x+1, y+0)
          };

          GLuint triangle1[3] =
          {
            getVertexIndex(x+1, y+1),
            getVertexIndex(x+0, y+1),
            getVertexIndex(x+0, y+0)
          };

          glm::vec3 normal0 = calcTriangleNormal(triangle0);
          for (unsigned int i = 0; i < 3; i++)
            setNormal(triangle0[i], glm::normalize(getNormal(triangle0[i]) + normal0));

          glm::vec3 normal1 = calcTriangleNormal(triangle1);
          for (unsigned int i = 0; i < 3; i++)
            setNormal(triangle1[i], glm::normalize(getNormal(triangle1[i]) + normal1));

          triangle_data_indexed.push_back(triangle0[0]);
          triangle_data_indexed.push_back(triangle0[1]);
          triangle_data_indexed.push_back(triangle0[2]);
          triangle_data_indexed.push_back(triangle1[0]);
          triangle_data_indexed.push_back(triangle1[1]);
          triangle_data_indexed.push_back(triangle1[2]);
        }
      }

      float triangles_size = triangle_data_indexed.size() * sizeof(GLuint) / 1024.0 / 1024.0;
      float vertices_size = vertices.size() * sizeof(Vertex) / 1024.0 / 1024.0;
      float normals_size = normals.size() * sizeof(Normal) / 1024.0 / 1024.0;
      float data_size = triangles_size + vertices_size + normals_size;
      cout<<"triangles_size: "<<triangles_size<<" MB"<<endl;
      cout<<"vertices_size: "<<vertices_size<<" MB"<<endl;
      cout<<"normals_size: "<<normals_size<<" MB"<<endl;
      cout<<"terrain data size total: "<<data_size<<" MB"<<endl;
    }
    
  };
  
}

struct render_util::Terrain::Private
{
  GLuint vao_id = 0;
  GLuint vertex_buffer_id = 0;
  GLuint normal_buffer_id = 0;
  GLuint index_buffer_id = 0;
  int num_vertices = 0;
  int num_indices = 0;
  TerrainMesh *mesh = 0;

  vector<glm::vec3> normals;

  ~Private() {
    deleteGLObjects();
    delete mesh;
  }
  
  void deleteGLObjects() {
    if (vao_id)
      gl::DeleteVertexArrays(1, &vao_id);
    vao_id = 0;

    if (vertex_buffer_id)
      gl::DeleteBuffers(1, &vertex_buffer_id);
    vertex_buffer_id = 0;

    if (normal_buffer_id)
      gl::DeleteBuffers(1, &normal_buffer_id);
    normal_buffer_id = 0;

    if (index_buffer_id)
      gl::DeleteBuffers(1, &index_buffer_id);
    index_buffer_id = 0;
  }
  
  void build(bool low_detail)
  {
    assert(mesh);

    deleteGLObjects();

    mesh->lod_level = 0;
    if (low_detail)
    {
      mesh->tile_factor = 32;
      mesh->lod_level = 7;
//       mesh->offset_x = -4 * map_width;
//       mesh->offset_y = -4 * map_height;
    }

    gl::GenBuffers(1, &vertex_buffer_id);
    assert(vertex_buffer_id > 0);
    gl::GenBuffers(1, &normal_buffer_id);
    assert(normal_buffer_id > 0);
    gl::GenBuffers(1, &index_buffer_id);
    assert(index_buffer_id > 0);

    gl::GenVertexArrays(1, &vao_id);
    assert(vao_id > 0);

    gl::BindVertexArray(vao_id);

    createVboIndexed();

    gl::BindVertexArray(0);

    gl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_id);
    gl::BufferData(GL_ELEMENT_ARRAY_BUFFER,
                mesh->triangle_data_indexed.size() * sizeof(GLuint),
                mesh->triangle_data_indexed.data(), GL_STATIC_DRAW);
    gl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);


    for (auto &n : mesh->normals)
      normals.push_back(glm::vec3(n.x, n.y, n.z));

    delete mesh;
    mesh = 0;
  }

  void createVboIndexed()
  {
    mesh->createTriangleDataIndexed();
    num_indices = mesh->triangle_data_indexed.size();

    cout<<"Terrain::Private::createVboIndexed()"<<endl;

    gl::BindBuffer(GL_ARRAY_BUFFER, vertex_buffer_id);
    gl::BufferData(GL_ARRAY_BUFFER,
                  mesh->vertices.size() * sizeof(Vertex),
                  mesh->vertices.data(), GL_STATIC_DRAW);
    gl::VertexPointer(3, GL_FLOAT, 0, 0);
    gl::EnableClientState(GL_VERTEX_ARRAY);
//     gl::VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
//     gl::EnableVertexAttribArray(0);

    CHECK_GL_ERROR();


    gl::BindBuffer(GL_ARRAY_BUFFER, normal_buffer_id);
    gl::BufferData(GL_ARRAY_BUFFER,
                  mesh->normals.size() * sizeof(Normal),
                  mesh->normals.data(), GL_STATIC_DRAW);
    gl::NormalPointer(GL_FLOAT, 0, 0);
    gl::EnableClientState(GL_NORMAL_ARRAY);

    CHECK_GL_ERROR();

    gl::BindBuffer(GL_ARRAY_BUFFER, 0);
  }
};


render_util::Terrain::Terrain() : p(new Private) {}

render_util::Terrain::~Terrain()
{
  delete p;
}

std::vector<glm::vec3> Terrain::getNormals()
{
  return p->normals;
}

void render_util::Terrain::build(ElevationMap::ConstPtr map)
{
  p->deleteGLObjects();
  delete p->mesh;
  p->mesh = new TerrainMesh;
  p->mesh->elevation_map = map;
  p->build(false);
}

void render_util::Terrain::draw()
{
  CHECK_GL_ERROR();

  auto program = getCurrentGLContext()->getCurrentProgram();
  assert(program);

  program->assertUniformsAreSet();

  gl::BindVertexArray(p->vao_id);
  CHECK_GL_ERROR();
  
  gl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, p->index_buffer_id);
  CHECK_GL_ERROR();

  gl::DrawElements(GL_TRIANGLES, p->num_indices, GL_UNSIGNED_INT, 0);
  CHECK_GL_ERROR();
  
  gl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  gl::BindVertexArray(0);
}

const string &render_util::Terrain::getName()
{
  static std::string name = "terrain";
  return name;
}

const util::Factory<TerrainBase> render_util::g_terrain_factory = util::makeFactory<Terrain>();
