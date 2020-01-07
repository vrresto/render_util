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

#include "terrain_cdlod_base.h"
#include "terrain_layer.h"
#include "textures.h"
#include "land_textures.h"
#include "forest_textures.h"
#include "water_textures.h"
#include "grid_mesh.h"
#include "vao.h"
#include <render_util/terrain_cdlod.h>
#include <render_util/texture_manager.h>
#include <render_util/texunits.h>
#include <render_util/elevation_map.h>
#include <render_util/texture_util.h>
#include <render_util/image.h>
#include <render_util/image_util.h>
#include <render_util/image_resample.h>
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
#include <log.h>


using namespace render_util::terrain;

using namespace render_util::gl_binding;
using namespace glm;

using render_util::TextureManager;
using render_util::TexturePtr;
using render_util::TerrainBase;
using render_util::TerrainCDLODBase;
using render_util::ImageRGB;
using render_util::ImageRGBA;
using render_util::ShaderProgramPtr;
using render_util::ShaderParameters;
using render_util::Rect;
using std::endl;
using std::vector;


namespace
{


class Material;

using MaterialID = render_util::TerrainBase::MaterialID;
using BoundingBox = render_util::Box;


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
  { 1800000, DetailOption::LAND },
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



TerrainTextureMap createHeightMap(unsigned int texunit, render_util::ElevationMap::ConstPtr image)
{
  TerrainTextureMap hm =
  {
    .texunit = texunit,
    .resolution_m = TerrainCDLODBase::HEIGHT_MAP_METERS_PER_GRID,
    .size_m = image->getSize() * (int)TerrainCDLODBase::HEIGHT_MAP_METERS_PER_GRID,
    .size_px = image->getSize(),
    .texture = TerrainCDLODBase::createHeightMapTexture(image),
    .name = "height_map",
  };
  return hm;
}

TerrainTextureMap createNormalMap(unsigned int texunit, render_util::ElevationMap::ConstPtr image)
{
  TerrainTextureMap nm =
  {
    .texunit = texunit,
    .resolution_m = TerrainCDLODBase::HEIGHT_MAP_METERS_PER_GRID,
    .size_m = image->getSize() * (int)TerrainCDLODBase::HEIGHT_MAP_METERS_PER_GRID,
    .size_px = image->getSize(),
    .texture = TerrainCDLODBase::createNormalMapTexture(image, TerrainCDLODBase::HEIGHT_MAP_METERS_PER_GRID),
    .name = "normal_map",
  };
  return nm;
}


render_util::ShaderProgramPtr createProgram(std::string name,
                                            unsigned int material,
                                            size_t detail_level,
                                            render_util::TextureManager &tex_mgr,
                                            const render_util::ShaderSearchPath &shader_search_path,
                                            const render_util::ShaderParameters &params_,
                                            bool enable_base_map)
{
  using namespace std;
  using namespace render_util;

  bool enable_base_water_map = false;
  bool is_editor = false;

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
  
  bool enable_type_map = false;
  bool enable_forest = false;

  if (material == MaterialID::WATER)
  {
//     params.set("enable_water", true);
//     params.set("enable_water_only", true);
  }
  else
  {
    enable_type_map = ((material & MaterialID::LAND)
//         && (detail_options & DetailOption::LAND)
      
    );
//     params.set("enable_water", material & MaterialID::WATER);
    enable_forest = (material & MaterialID::FOREST);
  }

//   params.set("detailed_water", detail_options & DetailOption::WATER);
//   params.set("detailed_forest", detail_options & DetailOption::FOREST);

  params.set("enable_type_map", enable_type_map);
  params.set("enable_forest", enable_forest);

  auto program = createShaderProgram(name, tex_mgr, shader_search_path, attribute_locations, params);

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
  Material(std::string program_name,
           unsigned int id, render_util::TextureManager &tm,
           const render_util::ShaderSearchPath &shader_search_path,
           const render_util::ShaderParameters &params,
           bool enable_base_map)
  {
    assert(m_batches.size() == NUM_DETAIL_LEVELS);
    for (size_t i = 0; i < m_batches.size(); i++)
    {
      assert(i < m_batches.size());
      auto program = createProgram(program_name, id, i, tm, shader_search_path, params, enable_base_map);
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


} // namespace


namespace render_util
{


class TerrainCDLOD : public TerrainCDLODBase
{
  TextureManager &texture_manager;

  ShaderSearchPath shader_search_path;

  NodeAllocator node_allocator;
  RenderList render_list;
  Node *root_node = 0;
  dvec2 root_node_pos = -dvec2(getNodeSize(MAX_LOD) / 2.0);

  std::unique_ptr<VertexArrayObject> vao;
  GLuint node_pos_buffer_id = 0;

  int num_indices = 0;
  float draw_distance = 0;

  vec2 m_base_map_origin = vec2(0);

  std::vector<TerrainLayer> m_layers;
  std::unordered_map<unsigned int, std::unique_ptr<Material>> materials;

  std::vector<std::unique_ptr<terrain::Textures>> m_textures;

  render_util::ShaderParameters m_shader_params;
  std::string m_program_name;

  void processNode(Node *node, int lod_level, const Camera &camera, bool low_detail);
  void drawInstanced(TerrainBase::Client *client);
  Node *createNode(const BuildParameters&, dvec2 pos, int lod_level, const MaterialMap&);
  void setUniforms(ShaderProgramPtr program);
  Material *getMaterial(unsigned int id);
  bool hasBaseMap();

public:
  TerrainCDLOD(TextureManager&, const ShaderSearchPath&);
  ~TerrainCDLOD() override;

  void build(BuildParameters&) override;
  void draw(TerrainBase::Client *client) override;
  void update(const Camera &camera, bool low_detail) override;
  void setDrawDistance(float dist) override;
  render_util::TexturePtr getNormalMapTexture() override;
  void setProgramName(std::string name) override;
  void setBaseMapOrigin(glm::vec2 origin) override;
};


TerrainCDLOD::~TerrainCDLOD()
{
  LOG_TRACE<<endl;

  render_list.clear();

  CHECK_GL_ERROR();

  gl::DeleteBuffers(1, &node_pos_buffer_id);

  CHECK_GL_ERROR();

  root_node = nullptr;
  node_allocator.clear();

  for (auto &t : m_textures)
    t->unbind(texture_manager);

  for (auto &layer : m_layers)
    layer.unbindTextures(texture_manager);

  CHECK_GL_ERROR();
}


TerrainCDLOD::TerrainCDLOD(TextureManager &tm, const ShaderSearchPath &shader_search_path) :
  texture_manager(tm),
  shader_search_path(shader_search_path)
{
  auto mesh = createGridMesh(MESH_GRID_SIZE+1, MESH_GRID_SIZE+1);

  num_indices = mesh.getNumIndices();

  gl::GenBuffers(1, &node_pos_buffer_id);
  assert(node_pos_buffer_id > 0);

  vao = std::make_unique<VertexArrayObject>(mesh, false);

  VertexArrayObjectBinding vao_binding(*vao);

  gl::BindBuffer(GL_ARRAY_BUFFER, node_pos_buffer_id);
  gl::VertexAttribPointer(4, 4, GL_FLOAT, false, 0, 0);
  gl::EnableVertexAttribArray(4);
  gl::BindBuffer(GL_ARRAY_BUFFER, 0);
  CHECK_GL_ERROR();
}


void TerrainCDLOD::setProgramName(std::string name)
{
  m_program_name = name;
}


Material *TerrainCDLOD::getMaterial(unsigned int id)
{
  auto it = materials.find(id);
  if (it != materials.end())
  {
    return it->second.get();
  }


  auto shader_params = m_shader_params;

  for (auto &t : m_textures)
    shader_params.add(t->getShaderParameters());

  materials[id] = std::make_unique<Material>(m_program_name,
                                             id, texture_manager, shader_search_path,
                                             shader_params,
                                             hasBaseMap());

  return materials[id].get();
}


void TerrainCDLOD::setUniforms(ShaderProgramPtr program)
{
  for (auto &t : m_textures)
    t->setUniforms(program);

  for (auto& layer : m_layers)
    layer.setUniforms(program, texture_manager);

  program->setUniform("cdlod_min_dist", MIN_LOD_DIST);

  program->setUniformi("terrain.mesh_resolution_m", GRID_RESOLUTION_M);

  program->setUniformi("terrain.tile_size_m", TILE_SIZE_M);
  program->setUniform("terrain.max_texture_scale", LandTextures::MAX_TEXTURE_SCALE);
}


Node *TerrainCDLOD::createNode(const BuildParameters &params,
                               dvec2 pos,
                               int lod_level,
                               const MaterialMap &material_map)
{
  assert(fract(pos) == dvec2(0));

  Node *node = node_allocator.alloc();
  node->pos = vec2(pos);
  node->pos_grid = vec2(pos / (double)METERS_PER_GRID);

  assert(fract(node->pos) == vec2(0));
  assert(fract(node->pos_grid) == vec2(0));

  double node_size = getNodeSize(lod_level);

  node->size = node_size;

  Rect area
  {
    .origin = vec2(pos),
    .extent = vec2(node_size),
  };

  if (lod_level == 0)
  {
    node->max_height = getMaxHeight(params, node->pos, node_size);
    node->material_id = material_map.getMaterialID(area);
  }
  else
  {
    double child_node_size = node_size / 2;
    assert(child_node_size == getNodeSize(lod_level-1));

    node->children[0] = createNode(params, pos + dvec2(0, child_node_size), lod_level-1, material_map);
    node->children[1] = createNode(params, pos + dvec2(child_node_size), lod_level-1, material_map);
    node->children[2] = createNode(params, pos + dvec2(0, 0), lod_level-1, material_map);
    node->children[3] = createNode(params, pos + dvec2(child_node_size, 0), lod_level-1, material_map);

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


void TerrainCDLOD::build(BuildParameters &params)
{
  CHECK_GL_ERROR();

  assert(!root_node);
  assert(m_layers.empty());

  m_shader_params = params.shader_parameters;

  CHECK_GL_ERROR();

  {
    auto land_textures = std::make_unique<LandTextures>(texture_manager, params);
    m_textures.push_back(std::move(land_textures));
  }

  {
    auto forest_textures = std::make_unique<ForestTextures>(texture_manager, params);
    m_textures.push_back(std::move(forest_textures));
  }

  terrain::WaterMap water_map;

  {
    auto water_textures = std::make_unique<WaterTextures>(texture_manager, params);
    water_map = water_textures->getWaterMap();
    m_textures.push_back(std::move(water_textures));
  }


  {
    LOG_INFO << "Creating detail layer ..." << endl;

    auto hm_image = params.loader.getDetailLayer().loadHeightMap();

    auto hm_image_resized = hm_image;
    auto new_size = glm::ceilPowerOfTwo(hm_image->size());
    if (new_size != hm_image->size())
    {
      hm_image_resized = image::extend(hm_image, new_size, 0.f, image::TOP_LEFT);
    }

    auto hm = ::createHeightMap(TEXUNIT_TERRAIN_CDLOD_HEIGHT_MAP, hm_image_resized);
    auto nm = ::createNormalMap(TEXUNIT_TERRAIN_CDLOD_NORMAL_MAP, hm_image);

    TerrainLayer layer;
    layer.origin_m = vec2(0);
    layer.size_m = vec2(hm_image->getSize() * (int)HEIGHT_MAP_METERS_PER_GRID);
    layer.uniform_prefix = "terrain.detail_layer.";
    LOG_INFO << "adding height map ..." << endl;
    layer.texture_maps.push_back(hm);
    LOG_INFO << "adding normal map ..." << endl;
    layer.texture_maps.push_back(nm);
    LOG_INFO << "adding water map ..." << endl;
    layer.water_map = water_map;
    LOG_INFO << "done." << endl;

    for (auto &t : m_textures)
    {
      for (auto &map : t->getTextureMaps())
        layer.texture_maps.push_back(map);
    }

    m_layers.push_back(layer);

    LOG_INFO << "Creating detail layer ... done." << endl;
  }

  if (params.loader.hasBaseLayer())
  {
    auto hm_image = params.loader.getBaseLayer().loadHeightMap();

    auto hm = ::createHeightMap(TEXUNIT_TERRAIN_CDLOD_HEIGHT_MAP_BASE, hm_image);
    auto nm = ::createNormalMap(TEXUNIT_TERRAIN_CDLOD_NORMAL_MAP_BASE, hm_image);

    TerrainLayer layer;
    layer.origin_m = m_base_map_origin;
    layer.size_m = vec2(hm_image->getSize() * (int)HEIGHT_MAP_METERS_PER_GRID);
    layer.uniform_prefix = "terrain.base_layer.";
    layer.texture_maps.push_back(hm);
    layer.texture_maps.push_back(nm);

    for (auto &t : m_textures)
    {
      for (auto &map : t->getBaseTextureMaps())
        layer.texture_maps.push_back(map);
    }

    m_layers.push_back(layer);
  }

  LOG_INFO<<"TerrainCDLOD: creating material map ..."<<endl;
  auto material_map = std::make_unique<MaterialMap>(params);
  LOG_INFO<<"TerrainCDLOD: creating material map ... done."<<endl;

  LOG_INFO<<"TerrainCDLOD: creating nodes ..."<<endl;
  root_node = createNode(params, root_node_pos, MAX_LOD, *material_map);
  LOG_INFO<<"TerrainCDLOD: creating nodes ... done."<<endl;

  LOG_INFO<<"TerrainCDLOD: done building terrain."<<endl;
}


void TerrainCDLOD::update(const Camera &camera, bool low_detail)
{
  assert(root_node);

  render_list.clear();

  processNode(root_node, MAX_LOD, camera, low_detail);

  constexpr int buffer_elements = getNumLeafNodes();
  constexpr int buffer_size = sizeof(RenderBatch::NodePos) * buffer_elements;

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


bool TerrainCDLOD::hasBaseMap()
{
  return m_layers.size() == 2;
}


void TerrainCDLOD::setBaseMapOrigin(glm::vec2 origin)
{
  m_base_map_origin = origin;
  if (hasBaseMap())
  {
    assert(m_layers.at(1).uniform_prefix == "terrain.base_layer.");
    m_layers.at(1).origin_m = origin;
  }
}


render_util::TexturePtr TerrainCDLOD::getNormalMapTexture()
{
  assert(!m_layers.empty());
  auto &map = m_layers.at(0).texture_maps.at(1); //FIXME
  assert(map.name == "normal_map");
  return map.texture;
}


void TerrainCDLOD::draw(Client *client)
{
  if (render_list.isEmpty())
    return;

  for (auto &t : m_textures)
    t->bind(texture_manager);

  for (auto& layer : m_layers)
    layer.bindTextures(texture_manager);

  assert(client);

  VertexArrayObjectBinding vao_binding(*vao);
  IndexBufferBinding index_buffer_binding(*vao);

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

  CHECK_GL_ERROR();
}


void TerrainCDLOD::setDrawDistance(float dist)
{
  draw_distance = dist;
}


const TerrainFactory g_terrain_cdlod_factory = makeTerrainFactory<TerrainCDLOD>();


} // namespace render_util
