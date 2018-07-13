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
#include <SOIL/SOIL.h>
#include <unistd.h>
#include "GL/gl.h"

// #include "texunits.h"
#include <gl_wrapper/gl_functions.h>

void doStuff(void *data);

//FIXME
#ifndef MAX_PATH
#define MAX_PATH 1024
#endif

using Clock = std::chrono::steady_clock;


using namespace gl_wrapper::gl_functions;
using namespace std;

namespace
{
  enum { NUM_ANIMATION_STEPS = 16 };

  const string &getWaterTextureDir()
  {
    static const string dir = render_util::getDataPath() + "/water_textures";
    return dir;
  }
}


namespace render_util
{


struct WaterAnimation::Private
{
  const float fps = 4;
  const int step_duration_microseconds =
    1000 * (1000.f / fps);

  size_t current_step = 0;
//   GLuint foam_detail_texture = 0;

  Clock::time_point last_step_time = Clock::now();
  Clock::time_point next_step_time =
    Clock::now() + std::chrono::microseconds(step_duration_microseconds);
  std::chrono::milliseconds delta_last_frame;

  void createTextures(MapTextures *map_textures)
  {
    printf("loading water textures...\n");

//     const char *foam_detail_name = "water_textures/FoamNV40.tga"; 
//     foam_detail_texture = SOIL_load_OGL_texture(foam_detail_name, SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID,
//         SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS);
// 
//     if (!foam_detail_texture) {
//       printf("Fail\n");
//       exit(1);
//     }
    
    
    vector<ImageRGBA::ConstPtr> normal_maps;
    vector<ImageGreyScale::ConstPtr> foam_masks;

    int i = 0;
    while (i < NUM_ANIMATION_STEPS)
    {
      char buf[MAX_PATH];

      snprintf(buf, sizeof(buf), "%s/WaterNoise%.2dDot3.tga", getWaterTextureDir().c_str(), i);
      printf("loading %s...\n", buf);
      
      ImageRGBA::Ptr normal_map(loadImageFromFile<ImageRGBA>(buf));
      if (!normal_map)
        break;
      
      normal_maps.push_back(normal_map);

      snprintf(buf, sizeof(buf), "%s/WaterNoiseFoam%.2d.tga", getWaterTextureDir().c_str(), i);
      printf("loading %s...\n", buf);

      ImageGreyScale::Ptr foam_mask(loadImageFromFile<ImageGreyScale>(buf));
      assert(foam_mask);

      foam_masks.push_back(foam_mask);

      i++;
    }

    assert(normal_maps.size() == NUM_ANIMATION_STEPS);

    map_textures->setTextureArray(TEXUNIT_WATER_NORMAL_MAP, normal_maps);
    map_textures->setTextureArray(TEXUNIT_FOAM_MASK, foam_masks);
  }

  float getFrameDelta()
  {
    float delta =  (float)delta_last_frame.count() / ((float)step_duration_microseconds / 1000.f);
//     std::cout<<delta<<'\n';
//     std::cout<< (float)delta_last_frame.count() / 1000.f << '\n';
//     delta = 0;
    
    return delta;
  }
  
  void update()
  {
    Clock::time_point now = Clock::now();
    if (now >= next_step_time) {
      last_step_time = next_step_time;
      next_step_time += std::chrono::microseconds(step_duration_microseconds);
      current_step++;
      current_step %= NUM_ANIMATION_STEPS;
    }

    delta_last_frame =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - last_step_time);
  }

};



WaterAnimation::WaterAnimation() : p(new Private) {}

WaterAnimation::~WaterAnimation()
{
  delete p;
}

void WaterAnimation::createTextures(MapTextures *map_textures)
{
  p->createTextures(map_textures);
}

int WaterAnimation::getCurrentStep()
{ 
  return p->current_step;
}

void WaterAnimation::update()
{
  p->update();
}
 
void WaterAnimation::updateUniforms(ShaderProgramPtr program)
{
  program->setUniformi("water_animation_num_frames", NUM_ANIMATION_STEPS);
  program->setUniformi("water_animation_pos", p->current_step);
  program->setUniform<float>("waterAnimationFrameDelta", p->getFrameDelta());
}
 
float WaterAnimation::getFrameDelta()
{
  return p->getFrameDelta();
}



} // namespace render_util


void doStuff(void *data)
{
}
