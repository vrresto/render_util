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
#include <render_util/globals.h>
#include <block_allocator.h>

#include <array>
#include <vector>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/round.hpp>

#include <GL/gl.h>
#include <gl_wrapper/gl_functions.h>


using namespace gl_wrapper::gl_functions;
using namespace glm;
using render_util::TexturePtr;
using std::cout;
using std::endl;
using std::vector;


namespace render_util
{
  class TerrainCDLOD : public TerrainBase
  {
    struct Private;
    Private *p = 0;

  public:
    TerrainCDLOD();
    ~TerrainCDLOD() override;

    const std::string &getName() override;
    void build(ElevationMap::ConstPtr map) override;
    void draw() override;
    void update(const Camera &camera) override;
    void setTextureManager(TextureManager*) override;
    void setDrawDistance(float dist) override;
    void setBaseElevationMap(ElevationMap::ConstPtr map) override;
  };
}


namespace
{


constexpr int METERS_PER_GRID = render_util::TerrainBase::GRID_RESOLUTION_M;


enum
{
  LOD_LEVELS = 8,
  MESH_GRID_SIZE = 64,
  MIN_LOD_DIST = 40000,

  HEIGHT_MAP_METERS_PER_GRID = 200,

  LEAF_NODE_SIZE = MESH_GRID_SIZE * METERS_PER_GRID,

  MAX_LOD = LOD_LEVELS,
};


enum class MaterialID
{
  LAND,
  WATER,
  LAND_WATER,
  LAND_FOREST,
  WATER_FOREST,
  LAND_WATER_FOREST
};


constexpr size_t getNumLeafNodes()
{
  return pow(4, (size_t)LOD_LEVELS);
}


constexpr float getLodLevelDist(int lod_level)
{
  return MIN_LOD_DIST * pow(2, lod_level);
}


float getMaxHeight(const render_util::ElevationMap &map, vec2 pos, float size)
{
  return 4000.0;
}


constexpr double getNodeSize(int lod_level)
{
  return pow(2, lod_level) * LEAF_NODE_SIZE;
}


constexpr double getNodeScale(int lod_level)
{
  return pow(2, lod_level);
}


static_assert(getNodeSize(0) == LEAF_NODE_SIZE);
static_assert(getNodeScale(0) == 1);


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
  vec2 pos_grid = vec2(0);
  float size = 0;
  float max_height = 0;
  BoundingBox bounding_box;
  MaterialID material = MaterialID::LAND;

  bool isInRange(const vec3 &camera_pos, float radius)
  {
    return bounding_box.getShortestDistance(camera_pos) <= radius;
  }
};


typedef util::BlockAllocator<Node, 1000> NodeAllocator;


struct RenderBatch
{
  struct NodePos
  {
    float x = 0;
    float y = 0;
    float z = 0;
    float w = 0;
  };

  MaterialID material = MaterialID::LAND;
  std::vector<vec2> positions;
  std::vector<int> lods;
  size_t node_pos_buffer_offset = 0;

  void addNode(Node *node, int lod)
  {
    positions.push_back(node->pos_grid);
    lods.push_back(lod);
  }

  size_t getSize() { return positions.size(); }
};


class RenderList
{
  std::vector<RenderBatch*> m_batches;
  std::unordered_map<MaterialID, RenderBatch*> m_materials;

public:
  RenderBatch *getBatch(MaterialID material)
  {
    if (!m_materials[material])
    {
      auto batch = new RenderBatch;
      m_batches.push_back(batch);
      m_materials[material] = batch;
    }
    return m_materials[material];
  }

  void addNode(Node *node, int lod_level)
  {
    RenderBatch *batch = getBatch(node->material);
    batch->addNode(node, lod_level);
  }

  void clear()
  {
    m_materials.clear();
    for (RenderBatch *batch : m_batches)
    {
      delete batch;
    }
    m_batches.clear();
  }

  int getNodeCount()
  {
    int count = 0;
    for (RenderBatch *b : m_batches)
      count += b->getSize();
    return count;
  }

  const std::vector<RenderBatch*> &getBatches() { return m_batches; }
};


TexturePtr createNormalMapTexture(render_util::ElevationMap::ConstPtr map, int meters_per_grid)
{
  using namespace render_util;

  cout<<"TerrainCDLOD: creating normal map ..."<<endl;
  auto normal_map = createNormalMap(map, meters_per_grid);
  cout<<"TerrainCDLOD: creating normal map done."<<endl;

  auto normal_map_texture =
    createFloatTexture(reinterpret_cast<const float*>(normal_map->getData()),
                      map->getWidth(),
                      map->getHeight(),
                      3);
  cout<<"TerrainCDLOD: creating normal map  texture done."<<endl;

  TextureParameters<int> params;
  params.set(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  params.set(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  params.apply(normal_map_texture);

  TextureParameters<glm::vec4> params_vec4;
  params_vec4.set(GL_TEXTURE_BORDER_COLOR, glm::vec4(0,0,1,0));
  params_vec4.apply(normal_map_texture);

  return normal_map_texture;
}


TexturePtr createHeightMapTexture(render_util::ElevationMap::ConstPtr hm_image)
{
  using namespace render_util;

  cout<<"TerrainCDLOD: creating height map texture ..."<<endl;

  auto height_map_texture = createFloatTexture(hm_image, true);
  assert(height_map_texture);
  CHECK_GL_ERROR();

  TextureParameters<int> params;
  params.set(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  params.set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  params.set(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  params.set(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  params.apply(height_map_texture);

  CHECK_GL_ERROR();

  return height_map_texture;
}


} // namespace


namespace render_util
{


struct TerrainCDLOD::Private
{
  TextureManager *texture_manager = 0;

  NodeAllocator node_allocator;

  RenderList render_list;
  Node *root_node = 0;
  dvec2 root_node_pos = -dvec2(getNodeSize(MAX_LOD) / 2.0);

  GLuint vao_id = 0;
  GLuint vertex_buffer_id = 0;
  GLuint index_buffer_id = 0;
  GLuint node_pos_buffer_id = 0;

  int num_indices = 0;
  float draw_distance = 0;

  TexturePtr height_map_texture;
  TexturePtr normal_map_texture;
  vec2 height_map_size_px = vec2(0);

  TexturePtr height_map_base_texture;
  TexturePtr normal_map_base_texture;
  vec2 height_map_base_size_px = vec2(0);

  Private();
  ~Private();
  void processNode(Node *node, int lod_level, const Camera &camera);
  void selectNode(Node *node, int lod_level);
  void drawInstanced();
  Node *createNode(const render_util::ElevationMap &map, dvec2 pos, int lod_level);
  void setUniforms(ShaderProgramPtr program);
};


TerrainCDLOD::Private::~Private()
{
  CHECK_GL_ERROR();

  gl::DeleteVertexArrays(1, &vao_id);
  gl::DeleteBuffers(1, &vertex_buffer_id);
  gl::DeleteBuffers(1, &index_buffer_id);

  root_node = 0;
  node_allocator.clear();

  CHECK_GL_ERROR();
}


TerrainCDLOD::Private::Private()
{
  GridMesh mesh(MESH_GRID_SIZE+1, MESH_GRID_SIZE+1);
  mesh.createTriangleDataIndexed();

  num_indices = mesh.triangle_data_indexed.size();

  gl::GenBuffers(1, &node_pos_buffer_id);
  assert(node_pos_buffer_id > 0);

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

  gl::BindBuffer(GL_ARRAY_BUFFER, node_pos_buffer_id);
  gl::VertexAttribPointer(4, 4, GL_FLOAT, false, 0, 0);
  gl::EnableVertexAttribArray(4);
  gl::BindBuffer(GL_ARRAY_BUFFER, 0);
  CHECK_GL_ERROR();

  gl::BindVertexArray(0);
  CHECK_GL_ERROR();

  gl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_id);
  gl::BufferData(GL_ELEMENT_ARRAY_BUFFER,
              mesh.triangle_data_indexed.size() * sizeof(GLuint),
              mesh.triangle_data_indexed.data(), GL_STATIC_DRAW);
  gl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  CHECK_GL_ERROR();
}


void TerrainCDLOD::Private::setUniforms(ShaderProgramPtr program)
{
  program->setUniform("terrain_resolution_m", (float)METERS_PER_GRID);

  program->setUniform("cdlod_min_dist", MIN_LOD_DIST);
  program->setUniform("cdlod_grid_size", vec2(MESH_GRID_SIZE));

  program->setUniform("height_map_size_m", height_map_size_px * (float)HEIGHT_MAP_METERS_PER_GRID);
  program->setUniform("height_map_size_px", height_map_size_px);

  program->setUniform("height_map_base_size_m", height_map_base_size_px * (float)HEIGHT_MAP_BASE_METERS_PER_PIXEL);

  program->setUniform("terrain_tile_size_m", (float)TILE_SIZE_M);
}


Node *TerrainCDLOD::Private::createNode(const render_util::ElevationMap &map,
                                        dvec2 pos,
                                        int lod_level)
{
  assert(fract(pos) == dvec2(0));

  Node *node = node_allocator.alloc();
  node->pos = vec2(pos);
  node->pos_grid = vec2(pos / (double)METERS_PER_GRID);

  assert(fract(node->pos) == vec2(0));
  assert(fract(node->pos_grid) == vec2(0));

  double node_size = getNodeSize(lod_level);

  node->size = node_size;

  if (lod_level == 0)
  {
    node->max_height = getMaxHeight(map, node->pos, node_size);
  }
  else
  {
    double child_node_size = node_size / 2;
    assert(child_node_size == getNodeSize(lod_level-1));

    node->children[0] = createNode(map, pos + dvec2(0, child_node_size), lod_level-1);
    node->children[1] = createNode(map, pos + dvec2(child_node_size, child_node_size), lod_level-1);
    node->children[2] = createNode(map, pos + dvec2(0, 0), lod_level-1);
    node->children[3] = createNode(map, pos + dvec2(child_node_size, 0), lod_level-1);

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
  render_list.addNode(node, lod_level);
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
  if (render_list.getNodeCount() == 0)
    return;

  gl::BindVertexArray(vao_id);
  CHECK_GL_ERROR();

  gl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_id);
  CHECK_GL_ERROR();

  gl::VertexAttribDivisor(4, 1);
  CHECK_GL_ERROR();

  assert(render_list.getNodeCount() > 0);

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


void TerrainCDLOD::build(ElevationMap::ConstPtr map)
{
  assert(p->texture_manager);
  assert(!p->root_node);

  CHECK_GL_ERROR();

  assert(!p->normal_map_texture);
  p->normal_map_texture = createNormalMapTexture(map, HEIGHT_MAP_METERS_PER_GRID);

  CHECK_GL_ERROR();

  cout<<"TerrainCDLOD: creating nodes ..."<<endl;
  p->root_node = p->createNode(*map, p->root_node_pos, MAX_LOD);
  cout<<"TerrainCDLOD: creating nodes done."<<endl;

  assert(!p->height_map_texture);
  cout<<"TerrainCDLOD: creating height map texture ..."<<endl;

  auto hm_image = map;
  auto new_size = glm::ceilPowerOfTwo(hm_image->size());

  if (new_size != hm_image->size())
    hm_image = image::extend(hm_image, new_size, 0.f, image::TOP_LEFT);

  p->height_map_size_px = hm_image->size();

  p->height_map_texture = createHeightMapTexture(hm_image);

  cout<<"TerrainCDLOD: done buildding terrain."<<endl;
}


void TerrainCDLOD::update(const Camera &camera)
{
  assert(p->root_node);

  p->render_list.clear();

  p->processNode(p->root_node, MAX_LOD, camera);

  const int buffer_elements = getNumLeafNodes();
  const int buffer_size = sizeof(RenderBatch::NodePos) * buffer_elements;

  unsigned int buffer_pos = 0;

  gl::BindBuffer(GL_ARRAY_BUFFER, p->node_pos_buffer_id);

  gl::BufferData(GL_ARRAY_BUFFER, buffer_size, 0, GL_STREAM_DRAW);

  RenderBatch::NodePos *buffer = (RenderBatch::NodePos*) gl::MapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
  assert(buffer);

  for (auto batch : p->render_list.getBatches())
  {
    batch->node_pos_buffer_offset = buffer_pos;

    for (int i = 0; i < batch->getSize(); i++)
    {
      assert(buffer_pos < buffer_elements);

      const int lod = batch->lods[i];

      RenderBatch::NodePos &pos = buffer[buffer_pos];
      pos.x = batch->positions[i].x;
      pos.y = batch->positions[i].y;
      pos.z = getNodeScale(lod);
      pos.w = getLodLevelDist(lod);

      buffer_pos++;
    }
  }


  buffer = nullptr;
  gl::UnmapBuffer(GL_ARRAY_BUFFER);

  gl::BindBuffer(GL_ARRAY_BUFFER, 0);
}

void TerrainCDLOD::draw()
{
  auto program = getCurrentGLContext()->getCurrentProgram();
  assert(program);

  p->texture_manager->bind(TEXUNIT_TERRAIN_CDLOD_NORMAL_MAP, p->normal_map_texture);
  p->texture_manager->bind(TEXUNIT_TERRAIN_CDLOD_HEIGHT_MAP, p->height_map_texture);

  if (p->normal_map_base_texture)
    p->texture_manager->bind(TEXUNIT_TERRAIN_CDLOD_NORMAL_MAP_BASE, p->normal_map_base_texture);
  if (p->height_map_base_texture)
    p->texture_manager->bind(TEXUNIT_TERRAIN_CDLOD_HEIGHT_MAP_BASE, p->height_map_base_texture);

  p->setUniforms(program);
  CHECK_GL_ERROR();
  program->assertUniformsAreSet();

  p->drawInstanced();

  CHECK_GL_ERROR();
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


void TerrainCDLOD::setBaseElevationMap(ElevationMap::ConstPtr map)
{
  p->height_map_base_size_px = map->size();
  p->height_map_base_texture = createHeightMapTexture(map);
  p->normal_map_base_texture = createNormalMapTexture(map, HEIGHT_MAP_BASE_METERS_PER_PIXEL);
}


const util::Factory<TerrainBase> g_terrain_cdlod_factory = util::makeFactory<TerrainCDLOD>();


} // namespace render_util
