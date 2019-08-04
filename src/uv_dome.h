#ifndef RENDER_UTIL_UV_DOME_H
#define RENDER_UTIL_UV_DOME_H

#include "indexed_mesh.h"

namespace render_util
{
  IndexedMesh generateUVDome(uint32_t meridians, uint32_t parallels, double angle);
}

#endif
