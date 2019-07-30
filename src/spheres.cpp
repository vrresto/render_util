// Oscar Sebio Cajaraville 2015
// caosdoar@gmail.com
//
// What sphere is the best?
// Or...
// For a number of triangles what sphere is closest to the perfect sphere?

/*
MIT License

Copyright (c) 2015 Oscar Sebio Cajaraville

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "spheres.h"
#include <util.h>

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <vector>
#include <map>
#include <random>
#include <string>
#include <iostream>
#include <fstream>

using util::PI;


namespace
{


void UVDome(uint32_t meridians, uint32_t parallels, double angle, spheres::IndexedMesh &mesh)
{
	mesh.vertices.emplace_back(0.0f, 1.0f, 0.0f);
	for (uint32_t j = 0; j < parallels - 1; ++j)
	{
    double pos = double(j+1) / double(parallels);
    pos = pow(pos, 2);

    double const polar = angle * pos;

		double const sp = std::sin(polar);
		double const cp = std::cos(polar);
		for (uint32_t i = 0; i < meridians; ++i)
		{
			double const azimuth = 2.0 * PI * double(i) / double(meridians);
			double const sa = std::sin(azimuth);
			double const ca = std::cos(azimuth);
			double const x = sp * ca;
			double const y = cp;
			double const z = sp * sa;
			mesh.vertices.emplace_back(x, y, z);
		}
	}

	for (uint32_t i = 0; i < meridians; ++i)
	{
		uint32_t const a = i + 1;
		uint32_t const b = (i + 1) % meridians + 1;
		mesh.addTriangle(0, b, a);
	}

	for (uint32_t j = 0; j < parallels - 2; ++j)
	{
		uint32_t aStart = j * meridians + 1;
		uint32_t bStart = (j + 1) * meridians + 1;
		for (uint32_t i = 0; i < meridians; ++i)
		{
			const uint32_t a = aStart + i;
			const uint32_t a1 = aStart + (i + 1) % meridians;
			const uint32_t b = bStart + i;
			const uint32_t b1 = bStart + (i + 1) % meridians;
			mesh.addQuad(a, a1, b1, b);
		}
	}
}


}


namespace spheres
{
  IndexedMesh generateUVDome(uint32_t meridians, uint32_t parallels, double angle)
  {
    IndexedMesh mesh;
    ::UVDome(meridians, parallels, angle, mesh);

    auto rot = glm::rotate(glm::mat4(1), glm::radians(90.f), glm::vec3(1.0, 0.0, 0.0));

    for (auto &v : mesh.vertices)
    {
      auto rotated = rot * glm::vec4(v.x, v.y, v.z, 1);
      v = glm::vec3(rotated);
    }

    return std::move(mesh);
  }
}
