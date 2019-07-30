#ifndef RENDER_UTIL_CIRRUS_CLOUDS_H
#define RENDER_UTIL_CIRRUS_CLOUDS_H

#include <render_util/camera.h>
#include <render_util/shader.h>
#include <render_util/texture_manager.h>

#include <memory>

namespace render_util
{

class CirrusClouds
{
  struct Impl;
  std::unique_ptr<Impl> impl;

public:
  CirrusClouds(TextureManager &txmgr, const ShaderSearchPath&, const ShaderParameters&);
  ~CirrusClouds();

  ShaderProgramPtr getProgram();

  void draw(const Camera &camera);
};


}


#endif
