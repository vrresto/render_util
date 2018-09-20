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

/**
 * This terrain implementation makes use of the technique described in the paper
 * "Continuous Distance-Dependent Level of Detail for Rendering Heightmaps (CDLOD)"
 * by Filip Strugar <http://www.vertexasylum.com/downloads/cdlod/cdlod_latest.pdf>.
 */

#include <render_util/terrain_cdlod.h>
#include <render_util/texture_manager.h>
#include <render_util/texunits.h>
#include <render_util/elevation_map.h>
#include <render_util/texture_util.h>
#include <render_util/image.h>
#include <render_util/image_util.h>
#include <render_util/water_map.h>
#include <render_util/render_util.h>
#include <render_util/gl_context.h>
#include <block_allocator.h>

#include <array>
#include <vector>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/round.hpp>

#include <GL/gl.h>
#include <gl_wrapper/gl_functions.h>


void doStuff(void *data);

#define DRAW_INSTANCED 1

using namespace gl_wrapper::gl_functions;
using namespace glm;
using render_util::TexturePtr;
using std::cout;
using std::endl;
using std::vector;


namespace render_util
{
  class ElevationMap;

  class TerrainCDLOD : public TerrainBase
  {
    struct Private;
    Private *p = 0;

  public:
    TerrainCDLOD();
    ~TerrainCDLOD() override;

    const std::string &getName() override;
    void build(const ElevationMap *map) override;
    void draw() override;
    void update(const Camera &camera) override;
    void setTextureManager(TextureManager*) override;
    void setDrawDistance(float dist) override;
  };
}


namespace
{

enum { NUM_TEST_BUFFERS = 4 };

enum
{
  HEIGHT_MAP_METERS_PER_GRID = 200,

//   METERS_PER_GRID = 200,
//   MAX_LOD = 10,
//   LEAF_NODE_SIZE = 1600,
//   MIN_LOD_DIST = 30000,

  METERS_PER_GRID = 200,
  MAX_LOD = 10,
  LEAF_NODE_SIZE = 6400,
  MIN_LOD_DIST = 40000,

//   METERS_PER_GRID = 100,
//   MAX_LOD = 12,
//   LEAF_NODE_SIZE = 3200,
//   LEAF_NODE_SIZE = 6400,

  MESH_GRID_SIZE = LEAF_NODE_SIZE / METERS_PER_GRID
};


struct GridMesh
{
  typedef render_util::Float3 Vertex;

  const int width = 0;
  const int height = 0;

  std::vector<Vertex> vertices;
  std::vector<GLuint> triangle_data_indexed;

  GridMesh(int width, int height) : width(width), height(height) {}

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
    cout<<"GridMesh::createVertexData()"<<endl;
    vertices.clear();

    cout<<"numVertices: "<<numVertices()<<endl;

    assert(vertexDataSize() > 0);
    vertices.resize(numVertices());

    for (int y = 0; y < height; y++)
    {
      for (int x = 0; x < width; x++)
      {
        vertices[getVertexIndex(x, y)] = { (float)x, (float)y, 0 };
      }
    }
  }

  void createTriangleDataIndexed()
  {
    cout<<"GridMesh::createTriangleDataIndexed()"<<endl;
    triangle_data_indexed.clear();
    createVertexData();

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
    float data_size = triangles_size + vertices_size;
    cout<<"triangles_size: "<<triangles_size<<" MB"<<endl;
    cout<<"vertices_size: "<<vertices_size<<" MB"<<endl;
    cout<<"terrain data size total: "<<data_size<<" MB"<<endl;
  }

};


struct BoundingBox : public render_util::Box
{
  float getShortestDistance(vec3 pos) const
  {
    float d = 0;
    for (auto &point : getCornerPoints())
    {
      if (d)
        d = min(d, distance(point, pos));
      else
        d = distance(point, pos);
    }
    return d;
  }
};


struct Node
{
  std::array<Node*, 4> children;
  vec2 pos = vec2(0);
  float size = 0;
  float max_height = 0;
  BoundingBox bounding_box;

  bool isInRange(const vec3 &camera_pos, float radius)
  {
    return bounding_box.getShortestDistance(camera_pos) <= radius;
  }
};


typedef util::BlockAllocator<Node, 1000> NodeAllocator;


struct RenderBatch
{
#if DRAW_INSTANCED
  struct NodePos
  {
    float x = 0; 
    float y = 0;
    float z = 0;
    float w = 0;
  };
//   std::array<NodePos, 400> positions;
//   int size = 0;
  std::vector<vec2> positions;
#else
  std::vector<vec2> positions;
#endif

  void addNode(Node *node);
  void prepare();
  void bind();

// #if DRAW_INSTANCED
//   size_t getSize() { return size; }
// #else
  size_t getSize() { return positions.size(); }
// #endif


  void clear()
  {
// #if DRAW_INSTANCED
//     size = 0;
// #else
    positions.clear();
// #endif
  }
};


void RenderBatch::addNode(Node *node)
{
// #if DRAW_INSTANCED
//   assert(size < positions.size());
//   positions[size] = { node->pos.x, node->pos.y };
//   size++;
// #else
  positions.push_back(node->pos);
// #endif
}

class RenderList
{
  std::array<RenderBatch, MAX_LOD+1> m_batches;

public:
  RenderBatch *getBatch(int lod_level)
  {
    return &m_batches[lod_level];
  }

  void clear()
  {
    for (RenderBatch &batch : m_batches)
      batch.clear();
  }

  int getNodeCount()
  {
    int count = 0;
    for (RenderBatch &b : m_batches)
      count += b.getSize();
    return count;
  }
};


float getLodLevelDist(int lod_level)
{
  return MIN_LOD_DIST * pow(2, lod_level);
}

float getMaxHeight(const render_util::ElevationMap &map, vec2 pos, float size)
{
  return 4000.0;
}

constexpr float getNodeSize(int lod_level)
{
  return pow(2, lod_level) * LEAF_NODE_SIZE;
}

constexpr float getNodeScale(int lod_level)
{
  return METERS_PER_GRID * pow(2, lod_level);
}

static_assert(getNodeSize(0) == LEAF_NODE_SIZE);
static_assert(getNodeScale(0) == METERS_PER_GRID);


} // namespace


namespace render_util
{

struct TerrainCDLOD::Private
{
  TextureManager *texture_manager = 0;

  NodeAllocator node_allocator;

  RenderList render_list;
  Node *root_node = 0;
  vec2 root_node_pos = -vec2(getNodeSize(MAX_LOD) / 2.0);

  GLuint vao_id = 0;
  GLuint vertex_buffer_id = 0;
  GLuint index_buffer_id = 0;
#if DRAW_INSTANCED
  GLuint node_pos_buffer_id = 0;
#endif

  int num_indices = 0;
  float draw_distance = 0;

  TexturePtr height_map_texture;
  TexturePtr normal_map_texture;

  vec2 height_map_size_m = vec2(0);

  GLuint test_buffer_id[NUM_TEST_BUFFERS] = { 0 };

  Private();
  ~Private();
  void processNode(Node *node, int lod_level, const Camera &camera);
  void selectNode(Node *node, int lod_level);
  void drawInstanced();
  Node *createNode(const render_util::ElevationMap &map, vec2 pos, int lod_level);
};

TerrainCDLOD::Private::~Private()
{
  CHECK_GL_ERROR();

  gl::DeleteVertexArrays(1, &vao_id);
  gl::DeleteBuffers(1, &vertex_buffer_id);
  gl::DeleteBuffers(1, &index_buffer_id);

  gl::DeleteBuffers(NUM_TEST_BUFFERS, test_buffer_id);

  root_node = 0;
  node_allocator.clear();

//   gl::DeleteBuffers(1, &normal_buffer_id);

  CHECK_GL_ERROR();
}

TerrainCDLOD::Private::Private()
{
  GridMesh mesh(MESH_GRID_SIZE+1, MESH_GRID_SIZE+1);
  mesh.createTriangleDataIndexed();

  num_indices = mesh.triangle_data_indexed.size();

  gl::GenBuffers(NUM_TEST_BUFFERS, test_buffer_id);

#if DRAW_INSTANCED
  gl::GenBuffers(1, &node_pos_buffer_id);
  assert(node_pos_buffer_id > 0);
#endif

  gl::GenBuffers(1, &vertex_buffer_id);
  assert(vertex_buffer_id > 0);
  gl::GenBuffers(1, &index_buffer_id);
  assert(index_buffer_id > 0);
  gl::GenVertexArrays(1, &vao_id);
  assert(vao_id > 0);

  gl::BindVertexArray(vao_id);

  gl::BindBuffer(GL_ARRAY_BUFFER, vertex_buffer_id);
  gl::BufferData(GL_ARRAY_BUFFER,
                mesh.vertices.size() * sizeof(GridMesh::Vertex),
                mesh.vertices.data(), GL_STATIC_DRAW);
  gl::VertexPointer(3, GL_FLOAT, 0, 0);
  gl::EnableClientState(GL_VERTEX_ARRAY);
  gl::BindBuffer(GL_ARRAY_BUFFER, 0);

#if DRAW_INSTANCED
  gl::BindBuffer(GL_ARRAY_BUFFER, node_pos_buffer_id);
  gl::VertexAttribPointer(4, 4, GL_FLOAT, false, 0, 0);
  gl::EnableVertexAttribArray(4);
  gl::BindBuffer(GL_ARRAY_BUFFER, 0);
  CHECK_GL_ERROR();
#endif

  gl::BindVertexArray(0);
  CHECK_GL_ERROR();

  gl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_id);
  gl::BufferData(GL_ELEMENT_ARRAY_BUFFER,
              mesh.triangle_data_indexed.size() * sizeof(GLuint),
              mesh.triangle_data_indexed.data(), GL_STATIC_DRAW);
  gl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  CHECK_GL_ERROR();
}

Node *TerrainCDLOD::Private::createNode(const render_util::ElevationMap &map, vec2 pos, int lod_level)
{
  Node *node = node_allocator.alloc();
  node->pos = pos;

  float node_size = getNodeSize(lod_level);

  node->size = node_size;

  if (lod_level == 0)
  {
    node->max_height = getMaxHeight(map, node->pos, node_size);
  }
  else
  {
    float child_node_size = node_size / 2;
    assert(child_node_size == getNodeSize(lod_level-1));

    node->children[0] = createNode(map, node->pos + vec2(0, child_node_size), lod_level-1);
    node->children[1] = createNode(map, node->pos + vec2(child_node_size, child_node_size), lod_level-1);
    node->children[2] = createNode(map, node->pos + vec2(0, 0), lod_level-1);
    node->children[3] = createNode(map, node->pos + vec2(child_node_size, 0), lod_level-1);

    for (Node *child : node->children)
    {
      node->max_height = max(node->max_height, child->max_height);
    }
  }

  auto bb_origin = vec3(node->pos, 0);
  auto bb_extent = vec3(node_size, node_size, node->max_height);
  node->bounding_box.set(bb_origin, bb_extent);

  return node;
}

void TerrainCDLOD::Private::selectNode(Node *node, int lod_level)
{
  render_list.getBatch(lod_level)->addNode(node);
}


void TerrainCDLOD::Private::processNode(Node *node, int lod_level, const Camera &camera)
{
  auto camera_pos = camera.getPos();

  if (camera.cull(node->bounding_box))
    return;

  if (lod_level == 0)
  {
    if (draw_distance > 0.0  && !node->isInRange(camera_pos, draw_distance))
      return;

    selectNode(node, 0);
  }
  else
  {
    if (node->isInRange(camera_pos, getLodLevelDist(lod_level-1)))
    {
      // select children
      for (Node *child : node->children)
      {
        processNode(child, lod_level-1, camera);
      }
    }
    else
    {
      if (draw_distance > 0.0  && !node->isInRange(camera_pos, draw_distance))
        return;

      selectNode(node, lod_level);
    }
  }
}


void TerrainCDLOD::Private::drawInstanced()
{
  gl::BindVertexArray(vao_id);
  CHECK_GL_ERROR();

  gl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_id);
  CHECK_GL_ERROR();

  gl::VertexAttribDivisor(4, 1);
  CHECK_GL_ERROR();

  gl::DrawElementsInstancedARB(GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, 0, render_list.getNodeCount());
  CHECK_GL_ERROR();

  gl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  gl::BindVertexArray(0);
}


TerrainCDLOD::TerrainCDLOD() : p(new Private) {}

TerrainCDLOD::~TerrainCDLOD()
{
  delete p;
}

void TerrainCDLOD::build(const ElevationMap *map)
{
  assert(p->texture_manager);
  assert(!p->root_node);

  CHECK_GL_ERROR();

  {
    assert(!p->normal_map_texture);

//     NormalMapCreator nm_creator;
//     nm_creator.grid_scale = 200;
//     nm_creator.elevation_map = *map;
//     nm_creator.normals.reset(new Image<Normal>(map->getSize()));
//     cout<<"TerrainCDLOD: creating normal map ..."<<endl;
//     nm_creator.calcNormals();
//     cout<<"TerrainCDLOD: creating normal map done."<<endl;
//     cout<<"TerrainCDLOD: creating normal map  texture ..."<<endl;

    cout<<"TerrainCDLOD: creating normal map ..."<<endl;
    Image<Normal>::Ptr normal_map = createNormalMap(*map, HEIGHT_MAP_METERS_PER_GRID);
    cout<<"TerrainCDLOD: creating normal map done."<<endl;

    p->normal_map_texture =
      createFloatTexture(reinterpret_cast<const float*>(normal_map->getData()),
                        map->getWidth(),
                        map->getHeight(),
                        3);
    cout<<"TerrainCDLOD: creating normal map  texture done."<<endl;

    TextureParameters<int> params;
    params.set(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    params.set(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    params.apply(p->normal_map_texture);

    TextureParameters<glm::vec4> params_vec4;
    params_vec4.set(GL_TEXTURE_BORDER_COLOR, glm::vec4(0,0,1,0));
    params_vec4.apply(p->normal_map_texture);
  }
  CHECK_GL_ERROR();

  p->texture_manager->bind(TEXUNIT_TERRAIN_CDLOD_NORMAL_MAP, p->normal_map_texture);

  cout<<"TerrainCDLOD: creating nodes ..."<<endl;
  p->root_node = p->createNode(*map, p->root_node_pos, MAX_LOD);
  cout<<"TerrainCDLOD: creating nodes done."<<endl;

  assert(!p->height_map_texture);
  cout<<"TerrainCDLOD: creating height map texture ..."<<endl;

  auto hm_image = map->toImage();
  auto new_size = glm::ceilPowerOfTwo(hm_image->size());

  if (new_size != hm_image->size())
    hm_image = image::extend(hm_image, new_size, 0.f, image::TOP_LEFT);

  p->height_map_texture = createFloatTexture(hm_image, true);

  assert(p->height_map_texture);
  cout<<"TerrainCDLOD: creating height map texture done."<<endl;
  p->texture_manager->bind(TEXUNIT_TERRAIN_CDLOD_HEIGHT_MAP, p->height_map_texture);
  CHECK_GL_ERROR();

  p->height_map_size_m = hm_image->size() * 200;

  TextureParameters<int> params;

  params.set(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  params.set(GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  params.set(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  params.set(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  params.apply(p->height_map_texture);

  CHECK_GL_ERROR();

//   {
//     TexturePtr node_pos_texture = Texture::create(GL_TEXTURE_1D);
//     TextureParameters<int> params;
//     params.set(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//     params.set(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//     params.apply(node_pos_texture);
//     CHECK_GL_ERROR();
// 
//     p->texture_manager->bind(TEXUNIT_TERRAIN_CDLOD_NODE_POS, node_pos_texture);
//     CHECK_GL_ERROR();
//   }

  cout<<"TerrainCDLOD: done buildding terrain."<<endl;
}

void TerrainCDLOD::update(const Camera &camera)
{
  assert(p->root_node);

//   unsigned int texture_save = 0;
//   gl::GetIntegerv(GL_TEXTURE_BINDING_1D, (int*)&texture_save);

  p->render_list.clear();

  p->processNode(p->root_node, MAX_LOD, camera);

#if 0
  for (int i = 0; i < NUM_TEST_BUFFERS; i++)
  {
    const int buffer_size = 4 * 2000 * 2000;
//     const int buffer_size = 1024;
    gl::BindBuffer(GL_ARRAY_BUFFER, p->test_buffer_id[i]);
    gl::BufferData(GL_ARRAY_BUFFER, buffer_size, 0, GL_STREAM_DRAW);

    char *buffer = (char*) gl::MapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    assert(buffer);
    
    doStuff(buffer);

  }
  
  for (int i = 0; i < NUM_TEST_BUFFERS; i++)
  {
    gl::BindBuffer(GL_ARRAY_BUFFER, p->test_buffer_id[i]);
    gl::UnmapBuffer(GL_ARRAY_BUFFER);
  }
#endif

#if DRAW_INSTANCED

  const int buffer_elements = 20000;
  const int buffer_size = sizeof(RenderBatch::NodePos) * buffer_elements;

  unsigned int buffer_pos = 0;

  gl::BindBuffer(GL_ARRAY_BUFFER, p->node_pos_buffer_id);

  gl::BufferData(GL_ARRAY_BUFFER, buffer_size, 0, GL_STREAM_DRAW);

  RenderBatch::NodePos *buffer = (RenderBatch::NodePos*) gl::MapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
  assert(buffer);

  for (int i = MAX_LOD; i >= 0; i--)
  {
    int scale = getNodeScale(i);
    float lod_distance = getLodLevelDist(i);

    RenderBatch *batch = p->render_list.getBatch(i);

    for (int i = 0; i < batch->getSize(); i++)
    {
      assert(buffer_pos < buffer_elements);

      RenderBatch::NodePos &pos = buffer[buffer_pos];
      pos.x = batch->positions[i].x;
      pos.y = batch->positions[i].y;
      pos.z = scale;
      pos.w = lod_distance;

      buffer_pos++;
    }
  }

  buffer = 0;
  gl::UnmapBuffer(GL_ARRAY_BUFFER);

  gl::BindBuffer(GL_ARRAY_BUFFER, 0);

#endif
}

void TerrainCDLOD::draw()
{
  auto program = getCurrentGLContext()->getCurrentProgram();
  assert(program);

#if DRAW_INSTANCED
  program->setUniform("cdlod_grid_size", vec2(MESH_GRID_SIZE));
  program->setUniform("height_map_size_m", p->height_map_size_m);
  CHECK_GL_ERROR();
  program->assertUniformsAreSet();

  p->drawInstanced();

  CHECK_GL_ERROR();
  return;
#endif

  assert(p->texture_manager);

  CHECK_GL_ERROR();

//   unsigned int active_unit_save;
//   gl::GetIntegerv(GL_ACTIVE_TEXTURE, reinterpret_cast<GLint*>(&active_unit_save));

//   gl::ActiveTexture(GL_TEXTURE0 + p->texture_manager->getTexUnitNum(TEXUNIT_TERRAIN_CDLOD_NODE_POS));

  gl::BindVertexArray(p->vao_id);
  CHECK_GL_ERROR();

  gl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, p->index_buffer_id);
  CHECK_GL_ERROR();

#if DRAW_INSTANCED
  gl::BindBuffer(GL_ARRAY_BUFFER, p->node_pos_buffer_id);
  CHECK_GL_ERROR();
#endif

#if DRAW_INSTANCED
  gl::VertexAttribDivisor(4, 1);
  CHECK_GL_ERROR();
#endif

  int draw_count = 0;

  program->setUniform("cdlod_grid_size", vec2(MESH_GRID_SIZE));
  program->setUniform("height_map_size_m", p->height_map_size_m);

  for (int i = MAX_LOD; i >= 0; i--)
  {
    RenderBatch *batch = p->render_list.getBatch(i);

//     p->texture_manager->bind(TEXUNIT_TERRAIN_CDLOD_NODE_POS, batch->texture, GL_TEXTURE_1D);

    int scale = getNodeScale(i);

//     int scale = getNodeScale(5); //FIXME

//     program->setUniform("cdlod_node_pos", vec2(i * 100000));

    program->setUniform("cdlod_node_scale", scale);

    float lod_distance = getLodLevelDist(i);
    program->setUniform("cdlod_lod_distance", lod_distance);


//   if (batch->getSize())
//     cout<<batch->getSize()<<endl;

#if DRAW_INSTANCED
    if (batch->getSize())
    {
//       gl::TexImage1D(GL_TEXTURE_1D,
//           0,
//           GL_RG32F,
//           batch->positions.size(),
//           0,
//           GL_RG,
//           GL_FLOAT,
// //           batch->positions.data()
//           (const float*)batch->positions.data()
//           );
//       CHECK_GL_ERROR();

//       gl::BufferData(GL_ARRAY_BUFFER,
//                      sizeof(RenderBatch::NodePos) * batch->positions.size(),
//                      batch->positions.data(),
//                      GL_STREAM_DRAW);


      const int buffer_elements = 1000;
      const int buffer_size = sizeof(RenderBatch::NodePos) * buffer_elements;

      assert(batch->getSize() <= buffer_elements);

      gl::BufferData(GL_ARRAY_BUFFER,
                     buffer_size,
                     0,
                     GL_STREAM_DRAW);

      RenderBatch::NodePos *buffer = (RenderBatch::NodePos*) gl::MapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
      assert(buffer);

//       memcpy(buffer, batch->positions.data(), sizeof(RenderBatch::NodePos) * batch->positions.size());
      for (int i = 0; i < batch->getSize(); i++)
      {
        RenderBatch::NodePos &pos = buffer[i];
        pos.x = batch->positions[i].x;
        pos.y = batch->positions[i].y;
        pos.z = scale;
      }

      buffer = 0;
      gl::UnmapBuffer(GL_ARRAY_BUFFER);

      CHECK_GL_ERROR();

      program->assertUniformsAreSet();
      CHECK_GL_ERROR();

//       unsigned int num_instances = min(batch->getSize(), 10u);

//       gl::DrawElements(GL_TRIANGLES, p->num_indices, GL_UNSIGNED_INT, 0);
      gl::DrawElementsInstancedARB(GL_TRIANGLES, p->num_indices, GL_UNSIGNED_INT, 0, batch->getSize());
//       gl::DrawElementsInstancedARB(GL_TRIANGLES, p->num_indices, GL_UNSIGNED_INT, 0, num_instances);
// //       gl::DrawArraysInstancedARB(GL_TRIANGLES, 0, p->num_indices, 10);
      CHECK_GL_ERROR();
    }
#else
    for (size_t i = 0; i < batch->getSize(); i++)
    {
      program->setUniform("cdlod_node_pos", batch->positions[i]);
      program->assertUniformsAreSet();
      gl::DrawElements(GL_TRIANGLES, p->num_indices, GL_UNSIGNED_INT, 0);
      draw_count++;
      CHECK_GL_ERROR();
    }
#endif

  }

//   cout<<"draw_count: "<<draw_count<<endl;

  CHECK_GL_ERROR();

  gl::BindBuffer(GL_ARRAY_BUFFER, 0);
  gl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  gl::BindVertexArray(0);

//   gl::ActiveTexture(active_unit_save);
//   CHECK_GL_ERROR();
}

void TerrainCDLOD::setTextureManager(TextureManager *m)
{
  p->texture_manager = m;
}

const std::string &render_util::TerrainCDLOD::getName()
{
  static std::string name = "terrain_cdlod";
  return name;
}

void TerrainCDLOD::setDrawDistance(float dist)
{
  p->draw_distance = dist;
}


const util::Factory<TerrainBase> g_terrain_cdlod_factory = util::makeFactory<TerrainCDLOD>();


} // namespace render_util
