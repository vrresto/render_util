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

#ifndef RENDER_UTIL_GRID_MESH_H
#define RENDER_UTIL_GRID_MESH_H

#include <render_util/geometry.h>

#include <vector>


namespace render_util
{


struct GridMesh
{
  typedef render_util::Float3 Vertex;

  const int width = 0;
  const int height = 0;

  std::vector<Vertex> vertices;
  std::vector<unsigned int> triangle_data_indexed;

  GridMesh(int width, int height) : width(width), height(height) {}

  int numVertices()
  {
    return width * height;
  }

  int vertexDataSize()
  {
    return numVertices() * sizeof(Vertex);
  }

  unsigned int getVertexIndex(int x, int y)
  {
    return y * width + x;
  }

  void createVertexData()
  {
//     cout<<"GridMesh::createVertexData()"<<endl;
    vertices.clear();

//     cout<<"numVertices: "<<numVertices()<<endl;

    assert(vertexDataSize() > 0);
    vertices.resize(numVertices());

    for (int y = 0; y < height; y++)
    {
      for (int x = 0; x < width; x++)
      {
        vertices[getVertexIndex(x, y)] = { (float)x, (float)y, 0 };
      }
    }
  }

  void createTriangleDataIndexed()
  {
//     cout<<"GridMesh::createTriangleDataIndexed()"<<endl;

    triangle_data_indexed.clear();
    createVertexData();

    for (int y = 0; y < height-1; y++)
    {
      for (int x = 0; x < width-1; x++)
      {
        unsigned int triangle0[3] =
        {
          getVertexIndex(x+1, y+1),
          getVertexIndex(x+0, y+0),
          getVertexIndex(x+1, y+0)
        };

        unsigned int triangle1[3] =
        {
          getVertexIndex(x+1, y+1),
          getVertexIndex(x+0, y+1),
          getVertexIndex(x+0, y+0)
        };

        triangle_data_indexed.push_back(triangle0[0]);
        triangle_data_indexed.push_back(triangle0[1]);
        triangle_data_indexed.push_back(triangle0[2]);
        triangle_data_indexed.push_back(triangle1[0]);
        triangle_data_indexed.push_back(triangle1[1]);
        triangle_data_indexed.push_back(triangle1[2]);
      }
    }

    float triangles_size = triangle_data_indexed.size() * sizeof(unsigned int) / 1024.0 / 1024.0;
    float vertices_size = vertices.size() * sizeof(Vertex) / 1024.0 / 1024.0;
    float data_size = triangles_size + vertices_size;
    
//     cout<<"triangles_size: "<<triangles_size<<" MB"<<endl;
//     cout<<"vertices_size: "<<vertices_size<<" MB"<<endl;
//     cout<<"terrain data size total: "<<data_size<<" MB"<<endl;
  }

};


} // namespace render_util

#endif
