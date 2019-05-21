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
#include "terrain_cdlod_base.h"
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

#include <set>
#include <deque>
#include <array>
#include <vector>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/round.hpp>

#include <GL/gl.h>
#include <render_util/gl_binding/gl_functions.h>

using namespace render_util::gl_binding;
using namespace glm;

using render_util::TextureManager;
using render_util::TexturePtr;
using render_util::TerrainBase;
using render_util::TerrainCDLODBase;
using render_util::ImageRGBA;
using render_util::ShaderProgramPtr;
using render_util::ShaderParameters;
using std::cout;
using std::endl;
using std::vector;


namespace
{


class Material;

using MaterialID = render_util::TerrainBase::MaterialID;
using BoundingBox = render_util::Box;



void createTextureArrays(std::vector<ImageRGBA::Ptr> &textures_in,
    const std::vector<float> &texture_scale_in,
    TerrainBase::TypeMap::ConstPtr type_map_in,
    double max_texture_scale,
    std::array<TexturePtr, render_util::MAX_TERRAIN_TEXUNITS> &arrays_out,
    TexturePtr &type_map_texture_out)
{
  cout<<"createTextureArrays<<"<<endl;
  
  using namespace glm;
  using namespace std;
  using namespace render_util;
  using TextureArray = vector<ImageRGBA::ConstPtr>;

  std::array<TextureArray, MAX_TERRAIN_TEXUNITS> texture_arrays;
  map<unsigned, glm::uvec3> mapping;

  std::set<size_t> all_texture_sizes;
  for (auto texture : textures_in)
  {
    if (!texture)
      continue;
    assert(texture->w() == texture->h());
    all_texture_sizes.insert(texture->w());
  }

  std::deque<size_t> texture_sizes;
  for (auto size : all_texture_sizes)
    texture_sizes.push_back(size);

  while (texture_sizes.size() > MAX_TERRAIN_TEXUNITS)
    texture_sizes.pop_front();

  auto smallest_size = texture_sizes.front();

  std::unordered_map<int, int> array_index_for_size;
  for (size_t i = 0; i < texture_sizes.size(); i++)
    array_index_for_size[texture_sizes.at(i)] = i;

  for (int i = 0; i < textures_in.size(); i++)
  {
    float scale = texture_scale_in.at(i);
    assert(scale != 0);
    assert(fract(scale) == 0);
    assert(scale <= max_texture_scale);
    scale += 128;
    assert(scale >= 0);
    assert(scale <= 255);

    auto image = textures_in.at(i);
    if (!image)
      continue;

    while (image->w() < smallest_size)
    {
//       auto biggest_size = texture_sizes.back();
//       cout<<"image->w(): "<<image->w()<<", smallest_size: "
//         <<smallest_size<<", biggest_size: "<<biggest_size<<endl;
      image = render_util::upSample(image, 2);
    }

    auto index = array_index_for_size.at(image->w());

    texture_arrays.at(index).push_back(image);
    mapping.insert(make_pair(i, glm::uvec3{index, texture_arrays[index].size()-1, scale}));

    textures_in.at(i).reset();
  }

  auto type_map = make_shared<ImageRGBA>(type_map_in->getSize());

  for (int y = 0; y < type_map->h(); y++)
  {
    for (int x = 0; x < type_map->w(); x++)
    {
      unsigned int orig_index = type_map_in->get(x,y) & 0x1F;

      uvec3 new_index(0);

      try
      {
        new_index = mapping.at(orig_index);
      }
      catch(...)
      {
        for (int i = 0; i < 4; i++)
        {
          try
          {
            new_index = mapping.at(orig_index - (orig_index % 4) + i);
            break;
          }
          catch(...)
          {
            new_index.x = 255;
          }
        }
      }

      assert(new_index.y <= 0x1F+1);
      type_map->at(x,y,0) = new_index.x;
      type_map->at(x,y,1) = new_index.y;
      type_map->at(x,y,2) = new_index.z;
      type_map->at(x,y,3) = 255;
    }
  }

  {
    TexturePtr t = render_util::createTexture<ImageRGBA>(type_map, false);
    TextureParameters<int> params;
    params.set(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    params.set(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    params.set(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    params.set(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    params.apply(t);
    type_map_texture_out = t;
  }

  
  for (int i = 0; i < texture_arrays.size(); i++)
  {
    CHECK_GL_ERROR();

    auto &textures = texture_arrays.at(i);
    if (textures.empty())
      continue;

    
    cout<<"array: "<<i<<endl;
    arrays_out.at(i) = render_util::createTextureArray<ImageRGBA>(textures);
    textures.clear();

    CHECK_GL_ERROR();
  }
}


class TerrainTextures
{
  const TextureManager &m_texture_manager;
  glm::ivec2 m_type_map_size = glm::ivec2(0);
  TexturePtr m_type_map_texture;
  TexturePtr m_type_map_texture_nm;
  ShaderParameters m_shader_params;
  std::array<TexturePtr, render_util::MAX_TERRAIN_TEXUNITS> m_textures;
  std::array<TexturePtr, render_util::MAX_TERRAIN_TEXUNITS> m_textures_nm;

public:
  static constexpr float MAX_TERRAIN_TEXTURE_SCALE = 8;

  TerrainTextures(const TextureManager &texture_manager,
                  std::vector<ImageRGBA::Ptr> &textures,
                  std::vector<ImageRGBA::Ptr> &textures_nm,
                  const std::vector<float> &texture_scale,
                  TerrainBase::TypeMap::ConstPtr type_map);

  const ShaderParameters &getShaderParameters() { return m_shader_params; }

  void setTextures(const std::vector<ImageRGBA::ConstPtr> &textures,
                          const std::vector<float> &texture_scale,
                          TerrainBase::TypeMap::ConstPtr type_map);

  void bind(TextureManager&);
  void setUniforms(ShaderProgramPtr program);
};


TerrainTextures::TerrainTextures(const TextureManager &texture_manager,
                                      std::vector<ImageRGBA::Ptr> &textures,
                                      std::vector<ImageRGBA::Ptr> &textures_nm,
                                      const std::vector<float> &texture_scale,
                                      TerrainBase::TypeMap::ConstPtr type_map_) :
  m_texture_manager(texture_manager),
  m_type_map_size(type_map_->getSize())
{
  using namespace glm;
  using namespace std;
  using namespace render_util;
  using TextureArray = vector<ImageRGBA::ConstPtr>;

  assert(textures.size() == textures_nm.size());
  assert(textures.size() == texture_scale.size());

  createTextureArrays(textures, texture_scale, type_map_, MAX_TERRAIN_TEXTURE_SCALE,
                      m_textures, m_type_map_texture);

  createTextureArrays(textures_nm, texture_scale, type_map_, MAX_TERRAIN_TEXTURE_SCALE,
                      m_textures_nm, m_type_map_texture_nm);

  for (int i = 0; i < m_textures.size(); i++)
  {
    CHECK_GL_ERROR();

    if (!m_textures.at(i))
      continue;

    m_shader_params.set(string( "enable_terrain") + to_string(i), true);
  }

  for (int i = 0; i < m_textures_nm.size(); i++)
  {
    CHECK_GL_ERROR();

    if (!m_textures_nm.at(i))
      continue;

    m_shader_params.set(string( "enable_terrain_detail_nm") + to_string(i), true);
  }
}


void TerrainTextures::bind(TextureManager &tm)
{
  using namespace render_util;

  tm.bind(TEXUNIT_TYPE_MAP, m_type_map_texture);

  for (int i = 0; i < m_textures.size(); i++)
  {
    CHECK_GL_ERROR();

    auto textures = m_textures.at(i);
    if (!textures)
      continue;

    auto texunit = TEXUNIT_TERRAIN + i;
    assert(texunit < TEXUNIT_NUM);

    tm.bind(texunit, textures);

    CHECK_GL_ERROR();
  }

  tm.bind(TEXUNIT_TYPE_MAP_NORMALS, m_type_map_texture_nm);

  for (int i = 0; i < m_textures_nm.size(); i++)
  {
    CHECK_GL_ERROR();

    auto textures = m_textures_nm.at(i);
    if (!textures)
      continue;

    auto texunit = TEXUNIT_TERRAIN_DETAIL_NM0 + i;
    assert(texunit < TEXUNIT_NUM);

    tm.bind(texunit, textures);

    CHECK_GL_ERROR();
  }
}


void TerrainTextures::setUniforms(ShaderProgramPtr program)
{
  assert(m_type_map_size != glm::ivec2(0));
  program->setUniform("typeMapSize", m_type_map_size);
}


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

  uvec2 new_size = uvec2(in->getSize()) / TerrainCDLODBase::MESH_GRID_SIZE;
  auto resized = std::make_shared<render_util::TerrainBase::MaterialMap>(new_size);

  for (int y = 0; y < resized->h(); y++)
  {
    for (int x = 0; x < resized->w(); x++)
    {
      uvec2 src_coords = uvec2(x,y) * TerrainCDLODBase::MESH_GRID_SIZE;
      uvec2 size = uvec2(TerrainCDLODBase::MESH_GRID_SIZE + 1);
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
    const uvec2 grid_pos = uvec2(node_origin) / (unsigned int)TerrainCDLODBase::LEAF_NODE_SIZE;

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


class TerrainCDLOD : public TerrainCDLODBase
{
  TextureManager &texture_manager;

  std::string shader_path;

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
//   TexturePtr normal_map_base_texture;
  vec2 height_map_base_size_px = vec2(0);

  std::unordered_map<unsigned int, std::unique_ptr<Material>> materials;

  std::unique_ptr<TerrainTextures> m_terrain_textures;

  void processNode(Node *node, int lod_level, const Camera &camera, bool low_detail);
  void drawInstanced(TerrainBase::Client *client);
  Node *createNode(const render_util::ElevationMap &map, dvec2 pos, int lod_level, MaterialMap::ConstPtr material_map);
  void setUniforms(ShaderProgramPtr program);
  Material *getMaterial(unsigned int id);

public:
  TerrainCDLOD(TextureManager&, std::string);
  ~TerrainCDLOD() override;

  void build(ElevationMap::ConstPtr, MaterialMap::ConstPtr, TypeMap::ConstPtr type_map,
             std::vector<ImageRGBA::Ptr>&,
             std::vector<ImageRGBA::Ptr>&,
             const std::vector<float>&) override;
  void draw(TerrainBase::Client *client) override;
  void update(const Camera &camera, bool low_detail) override;
  void setDrawDistance(float dist) override;
  void setBaseElevationMap(ElevationMap::ConstPtr map) override;
  render_util::TexturePtr getNormalMapTexture() override;
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

  assert(m_terrain_textures);

  materials[id] = std::make_unique<Material>(id, texture_manager, shader_path,
                                             m_terrain_textures->getShaderParameters());

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

  assert(m_terrain_textures);
  m_terrain_textures->setUniforms(program);
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


void TerrainCDLOD::build(ElevationMap::ConstPtr map,
                         MaterialMap::ConstPtr material_map,
                         TypeMap::ConstPtr type_map,
                         std::vector<ImageRGBA::Ptr> &textures,
                         std::vector<ImageRGBA::Ptr> &textures_nm,
                         const std::vector<float> &texture_scale)
{
  CHECK_GL_ERROR();

  assert(material_map);
  assert(!root_node);
  assert(!normal_map_texture);

  normal_map_texture = createNormalMapTexture(map, HEIGHT_MAP_METERS_PER_GRID);

  CHECK_GL_ERROR();

  m_terrain_textures =
    std::make_unique<TerrainTextures>(texture_manager, textures, textures_nm, texture_scale, type_map);

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


render_util::TexturePtr TerrainCDLOD::getNormalMapTexture()
{
  return normal_map_texture;
}


void TerrainCDLOD::draw(Client *client)
{
  if (render_list.isEmpty())
    return;

  m_terrain_textures->bind(texture_manager);

  texture_manager.bind(TEXUNIT_TERRAIN_CDLOD_NORMAL_MAP, normal_map_texture);
  texture_manager.bind(TEXUNIT_TERRAIN_CDLOD_HEIGHT_MAP, height_map_texture);

  //FIXME
//   if (normal_map_base_texture)
//     texture_manager.bind(TEXUNIT_TERRAIN_CDLOD_NORMAL_MAP_BASE, normal_map_base_texture);
//   if (height_map_base_texture)
//     texture_manager.bind(TEXUNIT_TERRAIN_CDLOD_HEIGHT_MAP_BASE, height_map_base_texture);

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

  CHECK_GL_ERROR();
}


void TerrainCDLOD::setDrawDistance(float dist)
{
  draw_distance = dist;
}


void TerrainCDLOD::setBaseElevationMap(ElevationMap::ConstPtr map)
{
  //FIXME
  assert(0);
//   height_map_base_size_px = map->size();
//   height_map_base_texture = createHeightMapTexture(map);
//   normal_map_base_texture = createNormalMapTexture(map, HEIGHT_MAP_BASE_METERS_PER_PIXEL);
}


const TerrainFactory g_terrain_cdlod_factory = makeTerrainFactory<TerrainCDLOD>();


} // namespace render_util
