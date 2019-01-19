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

#include <render_util/water.h>
#include <render_util/image.h>
#include <render_util/image_loader.h>
#include <render_util/map_textures.h>
#include <render_util/texunits.h>

#include <vector>
#include <chrono>
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include "GL/gl.h"


using Clock = std::chrono::steady_clock;

using namespace std;



namespace
{


constexpr int NUM_LAYERS = 2;

struct Layer
{
  const int step_duration_microseconds = 0;
  int current_step = 0;
  std::chrono::milliseconds delta_last_frame;

  Clock::time_point last_step_time;
  Clock::time_point next_step_time;

  Layer(float fps) : step_duration_microseconds(1000 * (1000.f / fps))
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


namespace render_util
{


struct WaterAnimation::Private
{
  array<Layer, NUM_LAYERS> layers { 2, 6 };

  int num_animation_steps = 0;

  const Layer &getLayer(size_t index) { return layers.at(index); }

  void createTextures(MapTextures *map_textures,
                      const vector<ImageRGBA::ConstPtr> &normal_maps,
                      const vector<ImageGreyScale::ConstPtr> &foam_masks)
  {
    assert(foam_masks.size() == normal_maps.size());

    num_animation_steps = normal_maps.size();

    map_textures->setTextureArray(TEXUNIT_WATER_NORMAL_MAP, normal_maps);
    map_textures->setTextureArray(TEXUNIT_FOAM_MASK, foam_masks);

  }


  void update()
  {
    assert(num_animation_steps);

    Clock::time_point now = Clock::now();

    for (Layer &l : layers)
    {
      if (now >= l.next_step_time)
      {
        l.last_step_time = l.next_step_time;
        l.next_step_time += std::chrono::microseconds(l.step_duration_microseconds);
        l.current_step++;
        l.current_step %= num_animation_steps;
      }

      l.delta_last_frame =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - l.last_step_time);
    }

  }

};


WaterAnimation::WaterAnimation() : p(new Private) {}

WaterAnimation::~WaterAnimation()
{
  delete p;
}

void WaterAnimation::createTextures(MapTextures *map_textures,
                                    const vector<ImageRGBA::ConstPtr> &normal_maps,
                                    const vector<ImageGreyScale::ConstPtr> &foam_masks)
{
  p->createTextures(map_textures, normal_maps, foam_masks);
}

void WaterAnimation::update()
{
  p->update();
}

void WaterAnimation::updateUniforms(ShaderProgramPtr program)
{
  program->setUniformi("water_animation_num_frames", p->num_animation_steps);

  for (int i = 0; i < NUM_LAYERS; i++)
  {
    string prefix = string("water_animation_params[") + to_string(i) + "].";

    program->setUniformi(prefix + "pos", p->getLayer(i).current_step);
    program->setUniform<float>(prefix + "frame_delta", p->getLayer(i).getFrameDelta());
  }
}

bool WaterAnimation::isEmpty()
{
  return !p->num_animation_steps;
}


} // namespace render_util
