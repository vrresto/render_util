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

#include "grid_mesh.h"
#include <render_util/terrain_cdlod.h>
#include <render_util/texture_manager.h>
#include <render_util/texunits.h>
#include <render_util/elevation_map.h>
#include <render_util/texture_util.h>
#include <render_util/image.h>
#include <render_util/image_util.h>
#include <render_util/water_map.h>
#include <render_util/shader_util.h>
#include <render_util/render_util.h>
#include <render_util/globals.h>
#include <block_allocator.h>

#include <array>
#include <vector>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/round.hpp>

#include <GL/gl.h>
#include <render_util/gl_binding/gl_functions.h>

using namespace render_util::gl_binding;
using namespace glm;
using render_util::TexturePtr;
using std::cout;
using std::endl;
using std::vector;


namespace
{


class Material;

using MaterialID = render_util::TerrainBase::MaterialID;
using BoundingBox = render_util::Box;

constexpr int METERS_PER_GRID = render_util::TerrainBase::GRID_RESOLUTION_M;
constexpr float MIN_LOD_DIST = 40000;

enum
{
  LOD_LEVELS = 8,
  MESH_GRID_SIZE = 64,

  HEIGHT_MAP_METERS_PER_GRID = 200,

  LEAF_NODE_SIZE = MESH_GRID_SIZE * METERS_PER_GRID,

  MAX_LOD = LOD_LEVELS,
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


struct DetailOption
{
  using Type = unsigned int;

  constexpr static Type LAND = 1;
  constexpr static Type WATER = 1 << 1;
  constexpr static Type FOREST = 1 << 2;
  constexpr static Type COAST = 1 << 3;
  constexpr static Type ALL = LAND | WATER | FOREST | COAST;
};

struct DetailLevel
{
  double distance = 0;
  unsigned int options = 0;
};

constexpr DetailLevel g_detail_levels[] =
{
  { 0, 0 },
  { 40000, DetailOption::LAND },
  { 15000, DetailOption::LAND | DetailOption::FOREST },
  { 5000, DetailOption::LAND | DetailOption::WATER | DetailOption::FOREST },
  { 2000, DetailOption::ALL },
};
constexpr size_t NUM_DETAIL_LEVELS = sizeof(g_detail_levels) / sizeof(DetailLevel);

const DetailLevel &getDetailLevel(size_t level)
{
  assert(level < NUM_DETAIL_LEVELS);
  return g_detail_levels[level];
}

size_t getDetailLevelAtDistance(double dist)
{
  for (size_t i = 0; i < NUM_DETAIL_LEVELS; i++)
  {
    if (getDetailLevel(i).distance >= dist)
      return i;
  }
  return 0;
}


render_util::ShaderProgramPtr createProgram(unsigned int material,
                                            size_t detail_level,
                                            render_util::TextureManager &tex_mgr,
                                            std::string shader_path,
                                            const render_util::ShaderParameters &params_)
{
  using namespace std;
  using namespace render_util;

  bool enable_base_map = false;
  bool enable_base_water_map = false;
  bool is_editor = false;

  string name;
  if (name.empty())
    name = "terrain";

  name += "_cdlod";

  auto detail_options = getDetailLevel(detail_level).options;

  CHECK_GL_ERROR();

  map<unsigned int, string>  attribute_locations = { { 4, "attrib_pos" } };

  ShaderParameters params = params_;
  params.set("enable_base_map", enable_base_map);
  params.set("enable_base_water_map", enable_base_water_map);
  params.set("is_editor", is_editor);

  if (material == MaterialID::WATER)
  {
    params.set("enable_water", true);
    params.set("enable_water_only", true);
  }
  else
  {
    params.set("enable_type_map", (material & MaterialID::LAND) &&
                                  (detail_options & DetailOption::LAND));
    params.set("enable_water", material & MaterialID::WATER);
    params.set("enable_forest", material & MaterialID::FOREST);
  }

  params.set("detailed_water", detail_options & DetailOption::WATER);
  params.set("detailed_forest", detail_options & DetailOption::FOREST);

  auto program = createShaderProgram(name, tex_mgr, shader_path, attribute_locations, params);

  CHECK_GL_ERROR();

  return program;
}


struct Node
{
  std::array<Node*, 4> children;
  vec2 pos = vec2(0);
  vec2 pos_grid = vec2(0);
  float size = 0;
  float max_height = 0;
  BoundingBox bounding_box;
  unsigned int material_id = 0;
  Material *material = nullptr;

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

  const render_util::ShaderProgramPtr program;
  std::vector<vec2> positions;
  std::vector<int> lods;
  size_t node_pos_buffer_offset = 0;
  bool is_active = false;

  RenderBatch(render_util::ShaderProgramPtr program) : program(program) {}

  void addNode(Node *node, int lod)
  {
    positions.push_back(node->pos_grid);
    lods.push_back(lod);
  }

  void clear()
  {
    positions.clear();
    lods.clear();
  }

  render_util::ShaderProgramPtr getProgram() {  return program; }
  size_t getSize() { return positions.size(); }
};


class Material
{
  std::array<std::unique_ptr<RenderBatch>, NUM_DETAIL_LEVELS> m_batches;

public:
  Material(unsigned int id, render_util::TextureManager &tm,
           std::string shader_path, const render_util::ShaderParameters &params)
  {
    assert(m_batches.size() == NUM_DETAIL_LEVELS);
    for (size_t i = 0; i < m_batches.size(); i++)
    {
      assert(i < m_batches.size());
      auto program = createProgram(id, i, tm, shader_path, params);
      m_batches[i] = std::make_unique<RenderBatch>(program);
    }
  }

  RenderBatch *getBatch(size_t detail_level)
  {
    assert(detail_level < m_batches.size());
    return m_batches[detail_level].get();
  }
};


class RenderList
{
  std::vector<RenderBatch*> m_active_batches;

public:
  ~RenderList()
  {
    clear();
  }

  void addNode(Node *node, int lod, size_t detail_level)
  {
    assert(node->material);

    auto batch = node->material->getBatch(detail_level);
    if (!batch->is_active)
    {
      m_active_batches.push_back(batch);
      batch->is_active = true;
    }
    batch->addNode(node, lod);
  }

  void clear()
  {
    for (auto &batch : m_active_batches)
    {
      batch->clear();
      batch->is_active = false;
    }
    m_active_batches.clear();
  }

  bool isEmpty() { return m_active_batches.empty(); }

  const std::vector<RenderBatch*> &getBatches() { return m_active_batches; }
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


unsigned int gatherMaterials(render_util::TerrainBase::MaterialMap::ConstPtr map,
                             uvec2 begin,
                             uvec2 size)
{
  assert(begin.x < map->w());
  assert(begin.y < map->h());

  uvec2 end = begin + size;
  end.x = min(end.x, (unsigned int)map->w()-1);
  end.y = min(end.y, (unsigned int)map->h()-1);

  unsigned int material = 0;

  for (int y = begin.y; y < end.y; y++)
  {
    for (int x = begin.x; x < end.x; x++)
    {
      material |= map->get(x,y);
    }
  }

  assert(material);

  return material;
}


render_util::TerrainBase::MaterialMap::Ptr
processMaterialMap(render_util::TerrainBase::MaterialMap::ConstPtr in)
{
  if (!in)
    return {};

  uvec2 new_size = uvec2(in->getSize()) / (unsigned int)MESH_GRID_SIZE;
  auto resized = std::make_shared<render_util::TerrainBase::MaterialMap>(new_size);

  for (int y = 0; y < resized->h(); y++)
  {
    for (int x = 0; x < resized->w(); x++)
    {
      uvec2 src_coords = uvec2(x,y) * (unsigned int)MESH_GRID_SIZE;
      uvec2 size = uvec2((unsigned int)MESH_GRID_SIZE + 1);
      if (src_coords.x > 0)
      {
        src_coords.x -= 1;
        size.x += 1;
      }
      if (src_coords.y > 0)
      {
        src_coords.y -= 1;
        size.y += 1;
      }
      resized->at(x,y) = gatherMaterials(in, src_coords, size);
    }
  }

  return resized;
}


unsigned int getMaterialID(render_util::TerrainBase::MaterialMap::ConstPtr material_map,
                         const glm::dvec2 &node_origin)
{
  if (material_map)
  {
    const uvec2 grid_pos = uvec2(node_origin) / (unsigned int)LEAF_NODE_SIZE;

    if (node_origin.x < 0 || node_origin.y < 0)
      return MaterialID::WATER;

    if (grid_pos.x >= material_map->w() || grid_pos.y >= material_map->h())
      return MaterialID::WATER;

    return material_map->get(grid_pos.x, grid_pos.y);
  }
  else
  {
    return MaterialID::ALL;
  }
}


} // namespace


namespace render_util
{


class TerrainCDLOD : public TerrainBase
{
  TextureManager &texture_manager;

  std::string shader_path;
  ShaderParameters shader_parameters;

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

  std::unordered_map<unsigned int, std::unique_ptr<Material>> materials;

  void processNode(Node *node, int lod_level, const Camera &camera, bool low_detail);
  void drawInstanced(TerrainBase::Client *client);
  Node *createNode(const render_util::ElevationMap &map, dvec2 pos, int lod_level, MaterialMap::ConstPtr material_map);
  void setUniforms(ShaderProgramPtr program);
  Material *getMaterial(unsigned int id);

public:
  TerrainCDLOD(TextureManager&, std::string);
  ~TerrainCDLOD() override;

  const std::string &getName() override;
  void build(ElevationMap::ConstPtr map, MaterialMap::ConstPtr material_map) override;
  void draw(TerrainBase::Client *client) override;
  void update(const Camera &camera, bool low_detail) override;
  void setDrawDistance(float dist) override;
  void setBaseElevationMap(ElevationMap::ConstPtr map) override;
  render_util::TexturePtr getNormalMapTexture() override;
  void setShaderParameters(const ShaderParameters&) override;
};


TerrainCDLOD::~TerrainCDLOD()
{
  render_list.clear();

  CHECK_GL_ERROR();

  gl::DeleteVertexArrays(1, &vao_id);
  gl::DeleteBuffers(1, &vertex_buffer_id);
  gl::DeleteBuffers(1, &index_buffer_id);

  root_node = 0;
  node_allocator.clear();

  CHECK_GL_ERROR();
}


TerrainCDLOD::TerrainCDLOD(TextureManager &tm, std::string shader_path) :
  texture_manager(tm),
  shader_path(shader_path)
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


Material *TerrainCDLOD::getMaterial(unsigned int id)
{
  auto it = materials.find(id);
  if (it != materials.end())
  {
    return it->second.get();
  }

  materials[id] = std::make_unique<Material>(id, texture_manager, shader_path, shader_parameters);

  return materials[id].get();
}


void TerrainCDLOD::setUniforms(ShaderProgramPtr program)
{
  program->setUniform("terrain_resolution_m", (float)METERS_PER_GRID);

  program->setUniform("cdlod_min_dist", MIN_LOD_DIST);
  program->setUniform("cdlod_grid_size", vec2(MESH_GRID_SIZE));

  program->setUniform("height_map_size_m", height_map_size_px * (float)HEIGHT_MAP_METERS_PER_GRID);
  program->setUniform("height_map_size_px", height_map_size_px);

  program->setUniform("height_map_base_size_m", height_map_base_size_px * (float)HEIGHT_MAP_BASE_METERS_PER_PIXEL);

  program->setUniform("terrain_tile_size_m", (float)TILE_SIZE_M);
}


Node *TerrainCDLOD::createNode(const render_util::ElevationMap &map,
                                        dvec2 pos,
                                        int lod_level,
                                        MaterialMap::ConstPtr material_map)
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
    node->material_id = ::getMaterialID(material_map, pos);
  }
  else
  {
    double child_node_size = node_size / 2;
    assert(child_node_size == getNodeSize(lod_level-1));

    node->children[0] = createNode(map, pos + dvec2(0, child_node_size), lod_level-1, material_map);
    node->children[1] = createNode(map, pos + dvec2(child_node_size), lod_level-1, material_map);
    node->children[2] = createNode(map, pos + dvec2(0, 0), lod_level-1, material_map);
    node->children[3] = createNode(map, pos + dvec2(child_node_size, 0), lod_level-1, material_map);

    for (Node *child : node->children)
    {
      node->max_height = max(node->max_height, child->max_height);
      node->material_id |= child->material_id;
    }
  }

  auto bb_origin = vec3(node->pos, 0);
  auto bb_extent = vec3(node_size, node_size, node->max_height);
  node->bounding_box.set(bb_origin, bb_extent);

  assert(node->material_id);
  node->material = getMaterial(node->material_id);

  return node;
}


void TerrainCDLOD::processNode(Node *node, int lod_level, const Camera &camera, bool low_detail)
{
  auto camera_pos = camera.getPos();

  if (camera.cull(node->bounding_box))
    return;

  size_t detail_level = 0;
  if (!low_detail)
  {
    for (size_t i = 0; i < NUM_DETAIL_LEVELS; i++)
    {
      if (node->isInRange(camera_pos, getDetailLevel(i).distance))
        detail_level = i;
    }
  }

  if (lod_level == 0)
  {
    if (draw_distance > 0.0  && !node->isInRange(camera_pos, draw_distance))
      return;

    render_list.addNode(node, 0, detail_level);
  }
  else
  {
    if (node->isInRange(camera_pos, getLodLevelDist(lod_level-1)))
    {
      // select children
      for (Node *child : node->children)
      {
        processNode(child, lod_level-1, camera, low_detail);
      }
    }
    else
    {
      if (draw_distance > 0.0  && !node->isInRange(camera_pos, draw_distance))
        return;

      render_list.addNode(node, lod_level, detail_level);
    }
  }
}


void TerrainCDLOD::drawInstanced(TerrainBase::Client *client)
{
  if (render_list.isEmpty())
    return;

  assert(client);

  gl::BindVertexArray(vao_id);
  CHECK_GL_ERROR();

  gl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_id);
  CHECK_GL_ERROR();

  gl::VertexAttribDivisor(4, 1);
  CHECK_GL_ERROR();

  gl::BindBuffer(GL_ARRAY_BUFFER, node_pos_buffer_id);
  gl::EnableVertexAttribArray(4);

  for (RenderBatch *batch : render_list.getBatches())
  {
    const size_t offset = batch->node_pos_buffer_offset * sizeof(RenderBatch::NodePos);

    auto program = batch->program;
    assert(program);
    client->setActiveProgram(program);
    setUniforms(program);
    CHECK_GL_ERROR();
    program->assertUniformsAreSet();

    gl::VertexAttribPointer(4, 4, GL_FLOAT, false, 0, (void*)offset);
    CHECK_GL_ERROR();

    gl::DrawElementsInstancedARB(GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, 0, batch->getSize());
    CHECK_GL_ERROR();
  }

  gl::BindBuffer(GL_ARRAY_BUFFER, 0);
  CHECK_GL_ERROR();

  gl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  gl::BindVertexArray(0);
}


void TerrainCDLOD::build(ElevationMap::ConstPtr map, MaterialMap::ConstPtr material_map)
{
  CHECK_GL_ERROR();

  assert(material_map);
  assert(!root_node);
  assert(!normal_map_texture);

  normal_map_texture = createNormalMapTexture(map, HEIGHT_MAP_METERS_PER_GRID);

  CHECK_GL_ERROR();

  cout<<"TerrainCDLOD: creating nodes ..."<<endl;
  root_node = createNode(*map, root_node_pos, MAX_LOD, processMaterialMap(material_map));
  cout<<"TerrainCDLOD: creating nodes done."<<endl;

  assert(!height_map_texture);
  cout<<"TerrainCDLOD: creating height map texture ..."<<endl;

  auto hm_image = map;
  auto new_size = glm::ceilPowerOfTwo(hm_image->size());

  if (new_size != hm_image->size())
    hm_image = image::extend(hm_image, new_size, 0.f, image::TOP_LEFT);

  height_map_size_px = hm_image->size();

  height_map_texture = createHeightMapTexture(hm_image);

  cout<<"TerrainCDLOD: done buildding terrain."<<endl;
}


void TerrainCDLOD::update(const Camera &camera, bool low_detail)
{
  assert(root_node);

  render_list.clear();

  processNode(root_node, MAX_LOD, camera, low_detail);

  const int buffer_elements = getNumLeafNodes();
  const int buffer_size = sizeof(RenderBatch::NodePos) * buffer_elements;

  size_t buffer_pos = 0;

  gl::BindBuffer(GL_ARRAY_BUFFER, node_pos_buffer_id);

  gl::BufferData(GL_ARRAY_BUFFER, buffer_size, 0, GL_STREAM_DRAW);

  RenderBatch::NodePos *buffer = (RenderBatch::NodePos*) gl::MapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
  assert(buffer);

  for (auto batch : render_list.getBatches())
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


render_util::TexturePtr TerrainCDLOD::getNormalMapTexture()
{
  return normal_map_texture;
}


void TerrainCDLOD::draw(Client *client)
{
  texture_manager.bind(TEXUNIT_TERRAIN_CDLOD_NORMAL_MAP, normal_map_texture);
  texture_manager.bind(TEXUNIT_TERRAIN_CDLOD_HEIGHT_MAP, height_map_texture);

  if (normal_map_base_texture)
    texture_manager.bind(TEXUNIT_TERRAIN_CDLOD_NORMAL_MAP_BASE, normal_map_base_texture);
  if (height_map_base_texture)
    texture_manager.bind(TEXUNIT_TERRAIN_CDLOD_HEIGHT_MAP_BASE, height_map_base_texture);

  drawInstanced(client);

  CHECK_GL_ERROR();
}


const std::string &render_util::TerrainCDLOD::getName()
{
  static std::string name = "terrain_cdlod";
  return name;
}


void TerrainCDLOD::setDrawDistance(float dist)
{
  draw_distance = dist;
}


void TerrainCDLOD::setBaseElevationMap(ElevationMap::ConstPtr map)
{
  height_map_base_size_px = map->size();
  height_map_base_texture = createHeightMapTexture(map);
  normal_map_base_texture = createNormalMapTexture(map, HEIGHT_MAP_BASE_METERS_PER_PIXEL);
}


void TerrainCDLOD::setShaderParameters(const ShaderParameters &params)
{
  shader_parameters = params;
}

const TerrainFactory g_terrain_cdlod_factory = makeTerrainFactory<TerrainCDLOD>();


} // namespace render_util
