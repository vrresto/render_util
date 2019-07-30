#ifndef SPHERES_H
#define SPHERES_H

#include <glm/glm.hpp>
#include <vector>

namespace spheres
{


using Index = unsigned int;

struct Triangle
{
  Index vertex[3];
};

using TriangleList=std::vector<Triangle>;
using VertexList=std::vector<glm::vec3>;


struct IndexedMesh
{
  VertexList vertices;
  TriangleList triangles;

  void addTriangle(uint32_t a, uint32_t b, uint32_t c)
  {
    triangles.push_back({a, b, c});
  }

  void addQuad(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
  {
    triangles.push_back({a, b, c});
    triangles.push_back({a, c, d});
  }
};


IndexedMesh generateUVDome(uint32_t meridians, uint32_t parallels, double angle);


}

#endif
