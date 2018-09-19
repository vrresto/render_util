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

#include "viewer_main.h"
#include "scene.h"
#include "camera.h"
// #include "texture.h"
#include <render_util/render_util.h>
#include <render_util/map.h>
#include <render_util/map_textures.h>
#include <render_util/terrain.h>
#include <render_util/terrain_cdlod.h>
#include <render_util/texture_util.h>
#include <render_util/texunits.h>
#include <render_util/image_loader.h>
#include <render_util/gl_context.h>
#include <render_util/camera.h>
#include <gl_wrapper/gl_wrapper.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <memory>

#include <gl_wrapper/gl_interface.h>
#include <gl_wrapper/gl_functions.h>

using namespace glm;
using namespace std;
using namespace gl_wrapper::gl_functions;
using namespace render_util::viewer;
using namespace render_util;
using render_util::ShaderProgram;
using Clock = std::chrono::steady_clock;


namespace
{
  const float camera_move_speed_default = 8000.0;
  const float camera_move_speed_min = 100.0;
  const float camera_move_speed_max = 10000.0;
  const float camera_rotation_speed = 45.0;
  const double mouse_rotation_speed = 0.2;

  std::shared_ptr<GLContext> g_context;
  std::shared_ptr<Scene> g_scene;

  float camera_move_speed = camera_move_speed_default;
  dvec2 last_cursor_pos(-1, -1);

  void printDistance (float distance, const char *suffix)
  {
    cout.width(10);
    cout<<distance<<suffix;
  }

  void printStats(int frame_delta_ms)
  {
    float fps = 1000.0 / frame_delta_ms;

    cout<<fixed;
    cout.fill(' ');

    // clear line (well - the 40 first characters)
    cout<<'\r';
    cout.width(40);
    cout<<"\r";

    cout.precision(3);

    cout<<"fps: "<<fps<< "   |   ";

    cout<<"pos: ";
    printDistance(g_scene->camera.x/1000, " ");
    printDistance(g_scene->camera.y/1000, " ");
    printDistance(g_scene->camera.z/1000, " ");

    cout<<"    |    speed: ";
    printDistance(camera_move_speed / 1000, " ");

    cout<<"   |    azimuth: "<<g_scene->sun_azimuth;

    cout.flush();
  }

  void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
  {
    if (action == GLFW_PRESS) {
      glfwGetCursorPos(window, &last_cursor_pos.x, &last_cursor_pos.y);
    }
    else if (action == GLFW_RELEASE) {
      last_cursor_pos = dvec2(-1, -1);
    }
  }

  void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
  {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
      glfwSetWindowShouldClose(window, true);
    }
    else if (key == GLFW_KEY_F2 && action == GLFW_PRESS) {
      g_scene->camera.fov += 10;
    }
    else if (key == GLFW_KEY_F3 && action == GLFW_PRESS) {
      g_scene->camera.fov -= 10;
    }
    else if (key == GLFW_KEY_R && action == GLFW_PRESS) {
      g_scene->camera.x = 0;
      g_scene->camera.y = 0;
      g_scene->camera.z = 0;

      camera_move_speed = camera_move_speed_default;
    }
    else if (key == GLFW_KEY_KP_ADD && action == GLFW_PRESS) {
      camera_move_speed *= 2;
    }
    else if (key == GLFW_KEY_KP_SUBTRACT && action == GLFW_PRESS) {
      camera_move_speed /= 2;
    }
    else if (key == GLFW_KEY_PAGE_UP && action == GLFW_PRESS) {
      if (mods & GLFW_MOD_SHIFT)
        g_scene->sun_azimuth += 1;
      else
        g_scene->sun_azimuth += 10;
    }
    else if (key == GLFW_KEY_PAGE_DOWN && action == GLFW_PRESS) {
      if (mods & GLFW_MOD_SHIFT)
        g_scene->sun_azimuth -= 1;
      else
        g_scene->sun_azimuth -= 10;
    }
    else if (action == GLFW_PRESS)
    {
      switch (key)
      {
        case GLFW_KEY_M:
          g_scene->toggle_lod_morph = !g_scene->toggle_lod_morph;
          cout<<endl<<"toggle_lod_morph: "<<g_scene->toggle_lod_morph<<endl;
          break;
        case GLFW_KEY_P:
          g_scene->pause_animations = !g_scene->pause_animations;
          break;
//         case GLFW_KEY_K:
// //           water_map_shift += (water_map_shift_unit / 2);
//           water_map_table_shift -= vec2(100, 0);
//           break;
//         case GLFW_KEY_L:
// //             water_map_shift -= (water_map_shift_unit / 2);
//           water_map_table_shift += vec2(100, 0);
//           break;
//         case GLFW_KEY_H:
//           water_map_table_shift -= vec2(0, 100);
//           break;
//         case GLFW_KEY_J:
//           water_map_table_shift += vec2(0, 100);
//           break;
      }
    }

  }

  void processInput(GLFWwindow *window, float frame_delta)
  {
//     const dvec2 viewport_size = g_scene->camera.getViewportSize();

    if (glfwGetMouseButton(window, 0) == GLFW_PRESS) {
      dvec2 cursor_pos;
      glfwGetCursorPos(window, &cursor_pos.x, &cursor_pos.y);
//       glfwSetCursorPos(window, viewport_size.x / 2, viewport_size.y / 2);

      if (last_cursor_pos != dvec2(-1, -1)) {
        dvec2 cursor_delta = last_cursor_pos - cursor_pos;
        last_cursor_pos = cursor_pos;

        dvec2 rotation = cursor_delta * mouse_rotation_speed;

        g_scene->camera.yaw += rotation.x;
        g_scene->camera.pitch += rotation.y;
      }
    }

    float move_speed = camera_move_speed;

//     if (glfwGetKey(window, GLFW_MOD_SHIFT) == GLFW_PRESS) {
//       move_speed /= 5;
//     }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
      g_scene->camera.moveForward(frame_delta * move_speed);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
      g_scene->camera.moveForward(frame_delta * -move_speed);
    }
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
      g_scene->camera.pitch -= camera_rotation_speed * frame_delta;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
      g_scene->camera.pitch += camera_rotation_speed * frame_delta;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
      g_scene->camera.yaw += camera_rotation_speed * frame_delta;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
      g_scene->camera.yaw -= camera_rotation_speed * frame_delta;
    }

//     vec4 direction = g_scene->camera.getDirection();
//     cout<<direction.x<<" "<<direction.y<<" "<<direction.z<<endl;

    g_scene->camera.updateTransform();
  }

  void *getGLProcAddress(const char *name)
  {
    return (void*) glfwGetProcAddress(name);
  }

  void errorCallback(int error, const char* description)
  {
    fprintf(stderr, "Error: %s\n", description);
    exit(1);
  }

} // namespace


shared_ptr<GLContext> render_util::getCurrentGLContext()
{
  return g_context;
}


void render_util::viewer::runApplication(util::Factory<Scene> f_create_scene)
{
  glfwSetErrorCallback(errorCallback);

  if (!glfwInit())
    exit(EXIT_FAILURE);

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
//   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
//   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

  glfwWindowHint(GLFW_VISIBLE, 0);

  GLFWwindow* window = glfwCreateWindow(1024, 768, "Test", NULL, NULL);
  if (!window)
  {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  glfwSetKeyCallback(window, keyCallback);
  glfwSetMouseButtonCallback(window, mouseButtonCallback);

  gl_wrapper::GL_Interface *gl_interface = new gl_wrapper::GL_Interface(&getGLProcAddress);
  gl_wrapper::GL_Interface::setCurrent(gl_interface);

  g_context = make_shared<GLContext>();

  g_scene = f_create_scene();
  assert(g_scene);

  g_scene->setup();

  CHECK_GL_ERROR();

//   gl::DepthMask(GL_TRUE);
//   gl::Enable (GL_DEPTH_TEST);
//   gl::FrontFace(GL_CCW);
  gl::CullFace(GL_BACK);
  gl::Enable(GL_CULL_FACE);
  gl::DepthFunc(GL_LEQUAL);

  glfwShowWindow(window);

#if GLFW_VERSION_MINOR >= 2
  glfwMaximizeWindow(window);
#endif

  Clock::time_point last_frame_time = Clock::now();
  Clock::time_point last_stats_time = Clock::now();

  while (!glfwWindowShouldClose(window))
  {
//     gl::Finish();

    CHECK_GL_ERROR();

    Clock::time_point current_frame_time = Clock::now();
    std::chrono::milliseconds frame_delta =
      std::chrono::duration_cast<std::chrono::milliseconds>(current_frame_time - last_frame_time);
    last_frame_time = current_frame_time;

    glfwPollEvents();
    processInput(window, (float)frame_delta.count() / 1000.0);

    if (glfwWindowShouldClose(window))
    {
      CHECK_GL_ERROR();
      break;
    }

    std::chrono::milliseconds stats_delta =
      std::chrono::duration_cast<std::chrono::milliseconds>(current_frame_time - last_stats_time);
    if (stats_delta.count() > 100) {
      last_stats_time = current_frame_time;
      printStats(frame_delta.count());
    }

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    gl::Viewport(0, 0, width, height);
    gl::DepthMask(GL_TRUE);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_ACCUM_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    g_scene->camera.setViewportSize(width, height);
    g_scene->camera.applyFov();

    g_scene->render((float)frame_delta.count() / 1000.0);

    CHECK_GL_ERROR();

    glfwSwapBuffers(window);

    CHECK_GL_ERROR();
  }

  cout<<endl;

  CHECK_GL_ERROR();

  g_scene.reset();
  g_context.reset();
  gl_wrapper::GL_Interface::setCurrent(nullptr);
  gl::Finish();

  CHECK_GL_ERROR();

  glfwMakeContextCurrent(0);

  glfwDestroyWindow(window);
  glfwTerminate();
}
