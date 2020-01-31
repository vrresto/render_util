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

#include "simple_viewer_application.h"
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
using namespace render_util;
using namespace render_util::viewer;
using namespace render_util::gl_binding;
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

  std::shared_ptr<SceneBase> g_scene;

  float camera_move_speed = camera_move_speed_default;
  dvec2 last_cursor_pos(-1, -1);
  bool g_shift_active = false;
  Clock::time_point g_last_frame_time {};
  std::unique_ptr<TextRenderer> g_text_renderer;


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


  void saveParameters()
  {
    cout << "-------------------" << endl;
    for (auto &p : g_scene->getParameters())
      cout << p->name << ": " << p->getValueString() << endl;
    cout << "-------------------" << endl;

    g_scene->save();
  }

  void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
  {
    if (action == GLFW_PRESS)
    {
      switch (key)
      {
        case GLFW_KEY_ESCAPE:
          glfwSetWindowShouldClose(window, true);
          break;
        case GLFW_KEY_F2:
          g_scene->camera.setFov(g_scene->camera.getFov() + 10);
          break;
        case GLFW_KEY_F3:
          g_scene->camera.setFov(g_scene->camera.getFov() - 10);
          break;
        case GLFW_KEY_KP_ADD:
          camera_move_speed *= 2;
          break;
        case GLFW_KEY_KP_SUBTRACT:
          camera_move_speed /= 2;
          break;
        case GLFW_KEY_P:
          saveParameters();
          break;
      }
    }

    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
      float step_factor = mods & GLFW_MOD_SHIFT ? 0.1 : 1;
      auto &p = g_scene->getActiveParameter();

      switch (key)
      {
        case GLFW_KEY_PAGE_UP :
          g_scene->setActiveParameter(g_scene->getActiveParameterIndex() + 1);
          break;
        case GLFW_KEY_PAGE_DOWN:
          g_scene->setActiveParameter(g_scene->getActiveParameterIndex() - 1);
          break;
        case GLFW_KEY_R:
          p.reset();
          break;
        case GLFW_KEY_LEFT:
          p.decrease(step_factor);
          break;
        case GLFW_KEY_RIGHT:
          p.increase(step_factor);
          break;
        case GLFW_KEY_UP:
          p.increase(step_factor, Parameter::Dimension::Y);
          break;
        case GLFW_KEY_DOWN:
          p.decrease(step_factor, Parameter::Dimension::Y);
          break;
      }
    }
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


  std::chrono::milliseconds frame()
  {
    Clock::time_point current_frame_time = Clock::now();
    std::chrono::milliseconds frame_delta =
      std::chrono::duration_cast<std::chrono::milliseconds>(current_frame_time - g_last_frame_time);
    g_last_frame_time = current_frame_time;
    return frame_delta;
  }


  void refresh(GLFWwindow* window, std::chrono::milliseconds frame_delta)
  {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    gl::Viewport(0, 0, width, height);
    gl::DepthMask(GL_TRUE);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_ACCUM_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    g_scene->camera.setViewportSize(width, height);

    g_scene->render((float)frame_delta.count() / 1000.0);

    ostringstream stats;
    printStats(frame_delta.count(), stats);

    g_text_renderer->SetColor(0,0,0);
    g_text_renderer->DrawText(stats.str(), 1, 1);
    g_text_renderer->SetColor(1.0, 1.0, 1.0);
    g_text_renderer->DrawText(stats.str(), 0, 0);

    for (int i = 0; i < g_scene->getParameters().size(); i++)
    {
      auto &parameter = *g_scene->getParameters().at(i);
      auto parameter_text = parameter.name + ": " + parameter.getValueString();

      float offset_y = i * 30;;

      g_text_renderer->SetColor(0,0,0);
      g_text_renderer->DrawText(parameter_text, 1, 31 + offset_y);

      if (i == g_scene->getActiveParameterIndex())
        g_text_renderer->SetColor(1,1,1);
      else
        g_text_renderer->SetColor(0.6, 0.6, 0.6);
      g_text_renderer->DrawText(parameter_text, 0, 30 + offset_y);
    }

    CHECK_GL_ERROR();

    glfwSwapBuffers(window);

    CHECK_GL_ERROR();
  }


  void refresh(GLFWwindow* window)
  {
    refresh(window, frame());
  }


} // namespace


void render_util::viewer::runSimpleApplication(util::Factory<SceneBase> f_create_scene, string app_name)
{
  initLog(app_name);

  glfwSetErrorCallback(errorCallback);

  if (!glfwInit())
    exit(EXIT_FAILURE);

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

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
  glfwSetWindowRefreshCallback(window, refresh);

  gl_binding::GL_Interface *gl_interface = new gl_binding::GL_Interface(&getGLProcAddress);
  gl_binding::GL_Interface::setCurrent(gl_interface);

  auto globals = make_shared<render_util::viewer::Globals>();

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

  g_text_renderer = make_unique<TextRenderer>();

  g_last_frame_time = Clock::now();

  while (!glfwWindowShouldClose(window))
  {
    CHECK_GL_ERROR();

    auto frame_delta = frame();

    glfwPollEvents();
    processInput(window, (float)frame_delta.count() / 1000.0);

    if (glfwWindowShouldClose(window))
    {
      CHECK_GL_ERROR();
      break;
    }

    refresh(window, frame_delta);
  }

  cout<<endl;

  CHECK_GL_ERROR();

  g_text_renderer.reset();

  g_scene.reset();
  globals.reset();
  gl::Finish();

  CHECK_GL_ERROR();

  gl_binding::GL_Interface::setCurrent(nullptr);

  glfwMakeContextCurrent(0);

  glfwDestroyWindow(window);
  glfwTerminate();
}
