#include "water.h"
#include <render_util/terrain_base.h>
#include <render_util/texunits.h>
#include <render_util/gl_binding/gl_functions.h>

#include <chrono>


using Clock = std::chrono::steady_clock;


namespace
{


const float water_map_chunk_size = 128;
const float water_map_crop_size = 4.0;
const float water_map_chunk_scale = 1.0 / (water_map_chunk_size /
                                           (water_map_chunk_size - (water_map_crop_size)));
const float water_map_shift_unit = 1.0 / (water_map_chunk_size - (water_map_crop_size));
const float water_map_chunk_shift_m = 1 * (water_map_shift_unit / 2);

const float water_map_chunk_size_m = 1600 * 4;

const glm::vec2 water_map_table_shift_m = glm::vec2(0, 200);


#if 0
float sampleShoreWave(float pos)
{
  const float peak_pos = 0.05;

  if (pos < peak_pos)
  {
    pos = pos /= peak_pos;
    return pow(pos, 2);
  }
  else
  {
    pos -= peak_pos;
    pos /= 1.0 - peak_pos;
    return pow(1 - pos, 8);
  }
}


ImageGreyScale::Ptr createShoreWaveTexture()
{
  LOG_INFO << "Creating shore wave texture ..." << endl;
  auto shore_wave = render_util::image::create<unsigned char>(0, glm::ivec2(4096, 1));
  assert(shore_wave);
  for (int i = 0; i < shore_wave->w(); i++)
  {
    float pos = 1.0 * i;
    pos /= shore_wave->w();
    float value = sampleShoreWave(pos);
    shore_wave->at(i,0) = value * 255;
  }
  LOG_INFO << "Creating shore wave texture ... done." << endl;
  return shore_wave;
}
#endif


constexpr int NUM_LAYERS = 2;

struct WaterAnimationLayer
{
  const int step_duration_microseconds = 0;
  int current_step = 0;
  std::chrono::milliseconds delta_last_frame;

  Clock::time_point last_step_time;
  Clock::time_point next_step_time;

  WaterAnimationLayer(float fps) : step_duration_microseconds(1000 * (1000.f / fps))
  {
    last_step_time = Clock::now();
    next_step_time = Clock::now() + std::chrono::microseconds(step_duration_microseconds);
  }

  float getFrameDelta() const
  {
    return  (float)delta_last_frame.count() / ((float)step_duration_microseconds / 1000.f);
  }
};


} // namespace


namespace render_util::terrain
{


struct WaterAnimation
{
  std::array<WaterAnimationLayer, NUM_LAYERS> m_layers { 2, 6 };
  int m_num_animation_steps = 0;

  void update();
  void setUniforms(ShaderProgramPtr program);
};


void WaterAnimation::update()
{
  if (!m_num_animation_steps)
    return;

  Clock::time_point now = Clock::now();

  for (auto &l : m_layers)
  {
    if (now >= l.next_step_time)
    {
      l.last_step_time = l.next_step_time;
      l.next_step_time += std::chrono::microseconds(l.step_duration_microseconds);
      l.current_step++;
      l.current_step %= m_num_animation_steps;
    }
    l.delta_last_frame =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - l.last_step_time);
  }
}


void WaterAnimation::setUniforms(ShaderProgramPtr program)
{
  assert(m_num_animation_steps > 0);

  program->setUniformi("water_animation_num_frames", m_num_animation_steps);

  for (int i = 0; i < NUM_LAYERS; i++)
  {
    auto prefix = std::string("water_animation_params[") + std::to_string(i) + "].";
    auto &layer = m_layers.at(i);

    program->setUniformi(prefix + "pos", layer.current_step);
    program->setUniform<float>(prefix + "frame_delta", layer.getFrameDelta());
  }
}


Water::Water(const TextureManager &texture_manager,
             render_util::TerrainBase::BuildParameters &params) :
    m_texture_manager(texture_manager)
{
  auto &normal_maps = params.loader.getWaterAnimationNormalMaps();
  auto &foam_masks = params.loader.getWaterAnimationFoamMasks();
  assert(normal_maps.size() == foam_masks.size());

  auto animation_steps = normal_maps.size();
  assert(animation_steps > 0);

  m_animation_normal_maps = createTextureArray(normal_maps);
  m_animation_foam_masks = createTextureArray(foam_masks);

  m_animation = std::make_unique<WaterAnimation>();
  m_animation->m_num_animation_steps = animation_steps;
}


Water::~Water()
{
}


void Water::bindTextures(TextureManager &tm)
{
  tm.bind(TEXUNIT_WATER_NORMAL_MAP, m_animation_normal_maps);
  tm.bind(TEXUNIT_FOAM_MASK, m_animation_foam_masks);
}


void Water::unbindTextures(TextureManager &tm)
{
  tm.unbind(TEXUNIT_WATER_NORMAL_MAP, m_animation_normal_maps->getTarget());
  tm.unbind(TEXUNIT_FOAM_MASK, m_animation_foam_masks->getTarget());
}


void Water::updateAnimation(float frame_delta)
{
  m_animation->update();
}


void Water::setUniforms(ShaderProgramPtr program) const
{
  program->setUniformi("sampler_water_normal_map",
                       m_texture_manager.getTexUnitNum(TEXUNIT_WATER_NORMAL_MAP));
  program->setUniformi("sampler_foam_mask",
                       m_texture_manager.getTexUnitNum(TEXUNIT_FOAM_MASK));
  program->setUniform("water_color", m_water_color);

  m_animation->setUniforms(program);
}


void Water::loadLayer(Layer &layer,
                      const TerrainBase::Loader::Layer &loader,
                      bool is_base_layer) const
{
  std::vector<ImageGreyScale::Ptr> chunks;
  Image<unsigned int>::Ptr table;

  try
  {
    loader.loadWaterMap(chunks, table);
  }
  catch (...)
  {
    return;
  }

  auto chunks_texture = createTextureArray(chunks);
  TextureParameters<int> chunks_params;
  chunks_params.set(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  chunks_params.set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  chunks_params.apply(chunks_texture);

  auto table_float = image::convert<float>(table);
  auto table_texture = createFloatTexture(
                                reinterpret_cast<const float*>(table_float->data()),
                                table_float->w(),
                                table_float->h(),
                                1);
  TextureParameters<int> table_params;
  table_params.set(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  table_params.set(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  table_params.set(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  table_params.set(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  table_params.apply(table_texture);

  WaterMap map
  {
    .texunit_table = is_base_layer ? TEXUNIT_WATER_MAP_TABLE_BASE : TEXUNIT_WATER_MAP_TABLE,
    .texunit = is_base_layer ? TEXUNIT_WATER_MAP_BASE : TEXUNIT_WATER_MAP,
    .texture_table = table_texture,
    .texture = chunks_texture,
    .table_size_m = glm::vec2(table->size()) * water_map_chunk_size_m,
    .table_shift_m = water_map_table_shift_m,
    .chunk_size_m = water_map_chunk_size_m,
    .chunk_scale = glm::vec2(water_map_chunk_scale),
    .chunk_shift_m = glm::vec2(water_map_chunk_shift_m) + water_map_table_shift_m,
  };

  layer.water_map = map;
  layer.has_water_map = true;
}


}
