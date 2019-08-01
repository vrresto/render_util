/**
 *    Rendering utilities
 *    Copyright (C) 2019  Jan Lepper
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

#include "grid_mesh.h"

using namespace render_util;


namespace
{


struct GridMeshCreator
{
  const int width = 0;
  const int height = 0;

  GridMeshCreator(int width, int height) : width(width), height(height) {}

  int numVertices()
  {
    return width * height;
  }

  IndexedMesh::Index getVertexIndex(int x, int y)
  {
    return y * width + x;
  }

  void createVertexData(IndexedMesh &mesh)
  {
    auto &vertices = mesh.vertices;

    vertices.clear();

    assert(numVertices() > 0);
    vertices.resize(numVertices());

    for (int y = 0; y < height; y++)
    {
      for (int x = 0; x < width; x++)
      {
        vertices.at(getVertexIndex(x, y)) = { (float)x, (float)y, 0 };
      }
    }
  }

  void createTriangleDataIndexed(IndexedMesh &mesh)
  {
    mesh.triangles.clear();

    createVertexData(mesh);

    for (int y = 0; y < height-1; y++)
    {
      for (int x = 0; x < width-1; x++)
      {
        mesh.addQuad(getVertexIndex(x+1, y+1),
                     getVertexIndex(x+1, y+0),
                     getVertexIndex(x+0, y+0),
                     getVertexIndex(x+0, y+1));
      }
    }
  }
};


}


render_util::IndexedMesh render_util::createGridMesh(int width, int height)
{
  render_util::IndexedMesh mesh;

  GridMeshCreator creator(width, height);
  creator.createTriangleDataIndexed(mesh);

  return std::move(mesh);
}
