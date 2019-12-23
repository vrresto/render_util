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
#include <text_renderer/text_renderer.h>
#include <render_util/render_util.h>
#include <render_util/map_textures.h>
#include <render_util/terrain.h>
#include <render_util/terrain_cdlod.h>
#include <render_util/texture_util.h>
#include <render_util/texunits.h>
#include <render_util/image_loader.h>
#include <render_util/gl_context.h>
#include <render_util/camera.h>
#include <render_util/gl_binding/gl_binding.h>
#include <log/file_appender.h>
#include <log/console_appender.h>
#include <log/txt_formatter.h>
#include <log/message_only_formatter.h>
#include <log.h>

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

using namespace glm;
using namespace std;
using namespace render_util::viewer;
using namespace render_util;
using render_util::ShaderProgram;
using Clock = std::chrono::steady_clock;


namespace render_util::viewer
{


class Globals : public render_util::Globals
{
  std::shared_ptr<render_util::GLContext> m_gl_context;

public:
  Globals() : m_gl_context(make_shared<render_util::GLContext>()) {}

  std::shared_ptr<render_util::GLContext> getCurrentGLContext() override
  {
    return m_gl_context;
  }
};


} // namespace render_util::viewer


namespace
{
  const float camera_move_speed_default = 8000.0;
  const float camera_move_speed_min = 100.0;
  const float camera_move_speed_max = 10000.0;
  const float camera_rotation_speed = 45.0;
  const double mouse_rotation_speed = 0.2;

  std::shared_ptr<Scene> g_scene;

  float camera_move_speed = camera_move_speed_default;
  dvec2 last_cursor_pos(-1, -1);
  bool g_shift_active = false;


  void initLog(string app_name)
  {
  #if USE_PLOG
    constexpr bool ADD_NEW_LINE = false;

    using namespace util::log;
    using FileSink = FileAppender<TxtFormatter<ADD_NEW_LINE>>;
    using ConsoleSink = ConsoleAppender<MessageOnlyFormatter<ADD_NEW_LINE>>;

    static FileSink file_sink_warn(app_name + "_warnings.log");
    static FileSink file_sink_info(app_name + "_info.log");
    static FileSink file_sink_debug(app_name + "_debug.log");
    static FileSink file_sink_trace(app_name + "_trace.log");

    static ConsoleSink console_sink;

    auto &logger_default = plog::init(plog::verbose);

    auto &warn_sink = plog::init<LOGGER_WARNING>(plog::warning, &file_sink_warn);
    auto &info_sink = plog::init<LOGGER_INFO>(plog::info, &file_sink_info);
    auto &debug_sink = plog::init<LOGGER_DEBUG>(plog::debug, &file_sink_debug);
    auto &trace_sink = plog::init<LOGGER_TRACE>(plog::verbose, &file_sink_trace);

    info_sink.addAppender(&console_sink);

    logger_default.addAppender(&warn_sink);
    logger_default.addAppender(&info_sink);
    logger_default.addAppender(&debug_sink);
    logger_default.addAppender(&trace_sink);
  #endif
  }


  void printDistance (float distance, const char *suffix, ostream &out)
  {
    out.width(8);
    out << distance << suffix;
  }


  void printStats(int frame_delta_ms, ostream &out)
  {
    out.precision(3);
    out.setf(ios_base::fixed);
    out.setf(ios_base::right);

    float fps = 1000.0 / frame_delta_ms;

    out << "fps: ";
    out.width(8);
    out << fps << "   |   ";

    out<<"pos: ";
    printDistance(g_scene->camera.x/1000, " ", out);
    printDistance(g_scene->camera.y/1000, " ", out);
    printDistance(g_scene->camera.z/1000, " ", out);

    out<<"    |    speed: ";
    printDistance(camera_move_speed / 1000, " ", out);

    out<<"   |  yaw: ";
    printDistance(g_scene->camera.yaw, " ", out);

    out<<"   |    sun elevation: ";
    printDistance(g_scene->sun_elevation, " ", out);
  }


  void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
  {
    if (action == GLFW_PRESS)
    {
      if (mods == GLFW_MOD_SHIFT)
        g_shift_active = true;
      else
        g_shift_active = false;
    }
    else if (action == GLFW_RELEASE)
    {
      g_shift_active = false;
    }

    if (button == 1)
    {
      if (action == GLFW_PRESS) {
        glfwGetCursorPos(window, &last_cursor_pos.x, &last_cursor_pos.y);
      }
      else if (action == GLFW_RELEASE) {
        last_cursor_pos = dvec2(-1, -1);
      }
    }
  }


  void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
  {
#if ENABLE_BASE_MAP
    auto base_map_origin_new = g_scene->base_map_origin;
#endif
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
      glfwSetWindowShouldClose(window, true);
    }
    else if (key == GLFW_KEY_F2 && action == GLFW_PRESS) {
      g_scene->camera.setFov(g_scene->camera.getFov() + 10);
    }
    else if (key == GLFW_KEY_F3 && action == GLFW_PRESS) {
      g_scene->camera.setFov(g_scene->camera.getFov() - 10);
    }

    else if (key == GLFW_KEY_F4 && action == GLFW_PRESS) {
      g_scene->setActiveController(0);
    }
    else if (key == GLFW_KEY_F5 && action == GLFW_PRESS) {
      g_scene->setActiveController(1);
    }
    else if (key == GLFW_KEY_F6 && action == GLFW_PRESS) {
      g_scene->setActiveController(2);
    }
    else if (key == GLFW_KEY_F7 && action == GLFW_PRESS) {
      g_scene->setActiveController(3);
    }
    else if (key == GLFW_KEY_F8 && action == GLFW_PRESS) {
      g_scene->setActiveController(4);
    }
    else if (key == GLFW_KEY_UP && action == GLFW_PRESS) {
      g_scene->setActiveController(g_scene->m_active_controller-1);
    }
    else if (key == GLFW_KEY_DOWN && action == GLFW_PRESS) {
      g_scene->setActiveController(g_scene->m_active_controller+1);
    }

    else if (key == GLFW_KEY_PAGE_UP && action == GLFW_PRESS &&
        g_scene->hasActiveController())
    {
      auto step_size = mods & GLFW_MOD_ALT ? 1.0 :
          mods & GLFW_MOD_SHIFT ? 0.01 :
            mods & GLFW_MOD_CONTROL ? 10.0 : 0.1;
      auto value = g_scene->getActiveController().get();
      g_scene->getActiveController().set(value + step_size);
    }
    else if (key == GLFW_KEY_PAGE_DOWN && action == GLFW_PRESS &&
        g_scene->hasActiveController())
    {
      auto step_size = mods & GLFW_MOD_ALT ? 1.0 :
          mods & GLFW_MOD_SHIFT ? 0.01 :
            mods & GLFW_MOD_CONTROL ? 10.0 : 0.1;
      auto value = g_scene->getActiveController().get();
      g_scene->getActiveController().set(value - step_size);
    }
    else if (key == GLFW_KEY_LEFT && (action == GLFW_PRESS || action == GLFW_REPEAT) &&
        g_scene->hasActiveController())
    {
      auto step_size = 0.01;
      auto value = g_scene->getActiveController().get();
      g_scene->getActiveController().set(value - step_size);
    }
    else if (key == GLFW_KEY_RIGHT && (action == GLFW_PRESS || action == GLFW_REPEAT) &&
        g_scene->hasActiveController())
    {
      auto step_size = 0.01;
      auto value = g_scene->getActiveController().get();
      g_scene->getActiveController().set(value + step_size);
    }

    else if (key == GLFW_KEY_R && action == GLFW_PRESS &&
        g_scene->hasActiveController())
    {
      g_scene->getActiveController().reset();
    }

    else if (key == GLFW_KEY_F9 && action == GLFW_PRESS)
    {
      auto &out = cout;

      out << endl;
      for (auto &c : g_scene->m_controllers)
      {
        out << c.name << ": " << c.get() << endl;
      }
      out << endl;
    }

    else if (key == GLFW_KEY_R && action == GLFW_PRESS &&
        mods & GLFW_MOD_ALT)
    {
      camera_move_speed = camera_move_speed_default;
    }
    else if (key == GLFW_KEY_F12 && action == GLFW_PRESS) {
      constexpr int num_channels = 3;

      int width, height;
      glfwGetFramebufferSize(window, &width, &height);

      vector<unsigned char> data(width * height * num_channels);
      gl::ReadnPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, data.size(), data.data());

      auto image = make_shared<ImageRGB>(ivec2(width, height), move(data));
      image = image::flipY(image);

      if (util::mkdir("il2ge_map_viewer"))
      {
        string path = "il2ge_map_viewer/screenshot_" + util::makeTimeStampString() + ".png";
        saveImageToFile(path, image.get(), ImageType::PNG);
      }
    }
    else if (key == GLFW_KEY_KP_ADD && action == GLFW_PRESS) {
      camera_move_speed *= 2;
    }
    else if (key == GLFW_KEY_KP_SUBTRACT && action == GLFW_PRESS) {
      camera_move_speed /= 2;
    }
    else if (key == GLFW_KEY_KP_8 && action == GLFW_PRESS) {
      if (mods & GLFW_MOD_CONTROL)
        g_scene->sun_elevation += 0.1;
      else if (mods & GLFW_MOD_SHIFT)
        g_scene->sun_elevation += 1;
      else
        g_scene->sun_elevation += 10;
    }
    else if (key == GLFW_KEY_KP_2 && action == GLFW_PRESS) {
      if (mods & GLFW_MOD_CONTROL)
        g_scene->sun_elevation -= 0.1;
      else if (mods & GLFW_MOD_SHIFT)
        g_scene->sun_elevation -= 1;
      else
        g_scene->sun_elevation -= 10;
    }
    else if (key == GLFW_KEY_KP_4 && action == GLFW_PRESS)
    {
      g_scene->sun_azimuth += 10;
    }
    else if (key == GLFW_KEY_KP_6 && action == GLFW_PRESS)
    {
      g_scene->sun_azimuth -= 10;
    }
    else if (action == GLFW_PRESS && mods == GLFW_MOD_SHIFT)
    {
#if ENABLE_BASE_MAP
      const int offset = 2000;

      switch (key)
      {
        case GLFW_KEY_LEFT:
          base_map_origin_new.x -= offset;
          break;
        case GLFW_KEY_RIGHT:
          base_map_origin_new.x += offset;
          break;
        case GLFW_KEY_UP:
          base_map_origin_new.y += offset;
          break;
        case GLFW_KEY_DOWN:
          base_map_origin_new.y -= offset;
          break;
      }
#endif
    }
    else if (action == GLFW_PRESS)
    {
#if ENABLE_BASE_MAP
      const int offset = 20000;

      switch (key)
      {
        case GLFW_KEY_B:
          g_scene->rebuild();
          break;
        case GLFW_KEY_LEFT:
          base_map_origin_new.x -= offset;
          break;
        case GLFW_KEY_RIGHT:
          base_map_origin_new.x += offset;
          break;
        case GLFW_KEY_UP:
          base_map_origin_new.y += offset;
          break;
        case GLFW_KEY_DOWN:
          base_map_origin_new.y -= offset;
          break;
        case GLFW_KEY_M:
          g_scene->toggle_lod_morph = !g_scene->toggle_lod_morph;
          cout<<endl<<"toggle_lod_morph: "<<g_scene->toggle_lod_morph<<endl;
          break;
        case GLFW_KEY_P:
          g_scene->pause_animations = !g_scene->pause_animations;
          break;
      }
#endif
    }
#if ENABLE_BASE_MAP
    if (base_map_origin_new != g_scene->base_map_origin)
    {
      g_scene->base_map_origin = base_map_origin_new;
      cout<<endl<<"base_map_origin: "<<g_scene->base_map_origin.x<<","<<g_scene->base_map_origin.y<<endl;
    }
#endif
  }

  void processInput(GLFWwindow *window, float frame_delta)
  {
    if (glfwGetMouseButton(window, 1) == GLFW_PRESS)
    {
      dvec2 cursor_pos;
      glfwGetCursorPos(window, &cursor_pos.x, &cursor_pos.y);

      if (last_cursor_pos != dvec2(-1, -1)) {
        dvec2 cursor_delta = last_cursor_pos - cursor_pos;
        last_cursor_pos = cursor_pos;

        dvec2 rotation = cursor_delta * mouse_rotation_speed;

        g_scene->camera.yaw += rotation.x;
        g_scene->camera.pitch += rotation.y;
      }
    }
    else if (glfwGetMouseButton(window, 0) == GLFW_PRESS)
    {
      dvec2 cursor_pos;
      glfwGetCursorPos(window, &cursor_pos.x, &cursor_pos.y);

      g_scene->cursorPos(cursor_pos);
      if (g_shift_active)
        g_scene->unmark();
      else
        g_scene->mark();
    }
    else
    {
      dvec2 cursor_pos;
      glfwGetCursorPos(window, &cursor_pos.x, &cursor_pos.y);

      assert(g_scene);
      g_scene->cursorPos(cursor_pos);
    }

    float move_speed = camera_move_speed;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
      g_scene->camera.moveForward(frame_delta * move_speed);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
      g_scene->camera.moveForward(frame_delta * -move_speed);
    }

//     if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
//       g_scene->camera.pitch -= camera_rotation_speed * frame_delta;
//     }
//     if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
//       g_scene->camera.pitch += camera_rotation_speed * frame_delta;
//     }
//     if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
//       g_scene->camera.yaw += camera_rotation_speed * frame_delta;
//     }
//     if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
//       g_scene->camera.yaw -= camera_rotation_speed * frame_delta;
//     }

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


void render_util::viewer::runApplication(util::Factory<Scene> f_create_scene, string app_name)
{
  initLog(app_name);

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

  gl_binding::GL_Interface *gl_interface = new gl_binding::GL_Interface(&getGLProcAddress);
  gl_binding::GL_Interface::setCurrent(gl_interface);

  auto globals = make_shared<render_util::viewer::Globals>();

  g_scene = f_create_scene();
  assert(g_scene);

  LOG_INFO << "Setting up scene ..." << endl;
  g_scene->setup();
  LOG_INFO << "Setting up scene ... done." << endl;

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

  auto text_renderer = make_unique<TextRenderer>();

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

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    gl::Viewport(0, 0, width, height);
    gl::DepthMask(GL_TRUE);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_ACCUM_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    g_scene->camera.setViewportSize(width, height);

    g_scene->render((float)frame_delta.count() / 1000.0);

    ostringstream stats;
    printStats(frame_delta.count(), stats);

    text_renderer->SetColor(0,0,0);
    text_renderer->DrawText(stats.str(), 1, 1);
    text_renderer->SetColor(1.0, 1.0, 1.0);
    text_renderer->DrawText(stats.str(), 0, 0);


    for (int i = 0; i < g_scene->m_controllers.size(); i++)
    {
      auto &controller = g_scene->m_controllers.at(i);

      char buf[100];
      snprintf(buf, sizeof(buf), "%.2f", controller.get());

      auto controller_text = controller.name + ": " + buf;

      float offset_y = i * 30;;

      text_renderer->SetColor(0,0,0);
      text_renderer->DrawText(controller_text, 1, 31 + offset_y);

      if (i == g_scene->m_active_controller)
        text_renderer->SetColor(1,1,1);
      else
        text_renderer->SetColor(0.6, 0.6, 0.6);
      text_renderer->DrawText(controller_text, 0, 30 + offset_y);
    }

    CHECK_GL_ERROR();

    glfwSwapBuffers(window);

    CHECK_GL_ERROR();
  }

  cout<<endl;

  CHECK_GL_ERROR();

  text_renderer.reset();

  g_scene.reset();
  globals.reset();
  gl::Finish();

  CHECK_GL_ERROR();

  gl_binding::GL_Interface::setCurrent(nullptr);

  glfwMakeContextCurrent(0);

  glfwDestroyWindow(window);
  glfwTerminate();
}
