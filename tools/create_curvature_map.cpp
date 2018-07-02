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

#include <iostream>
#include <fstream>
#include <cmath>
#include <GL/gl.h>

#include <distances.h>
#include "curvature_map.h"

using namespace std;
using namespace glm;

namespace {
  
//   const long double PI = 3.1415926535897932384626433832795L;
//   const long double PI = std::acos((long double)-1.0);

//   const double planet_radius = 1500 * 1000;
//   const long double planet_radius = 6371 * 1000;
//   const long double circumference = 2.0 * PI * planet_radius;
//   const double atmosphereHeight = 140.0 * 1000.0;
  const long double circumference = planet_circumference;
  
//   const float max_distance = 1500.0 * 1000.0;
  const float max_distance = curvature_map_max_distance;
//   const float max_distance = circumference / 4.0;
  
  const int map_width = curvature_map_width;
  const int map_height = curvature_map_height;
  const int size = curvature_map_size;
  const int size_bytes = curvature_map_size_bytes;
  
  struct float2 {
    GLfloat x = 0;
    GLfloat y = 0;
  };

  float2 map[curvature_map_width][curvature_map_height];

  template<typename T>
  bool equals(const T &a, const T &b, const T &epsilon)
  {
    return abs(a - b) < epsilon;
  }

  template<typename T>
  bool lessOrEqual(const T &a, const T &b, const T &epsilon)
  {
    return a < b || equals(a, b, epsilon);
  }

//   dvec2 rotate(dvec2 v, long double a) {
//     long double s = sin(a);
//     long double c = cos(a);
//     dmat2 m = dmat2(c, -s, s, c);
//     return m * v;
//   }

//   dvec2 rotate(dvec2 v, Complex128 a) {
//     long double c = std::cos(a);
//     long double s = std::sin(a);
//     
//     long double x = v.x;
//     long double y = v.y;
// 
//     dvec2 res;
//     res.x = x * c - y * s; 
//     res.y = x * s + y * c;
// 
//     return res;
//   }


  void rotate(long double x, long double y, long double a, long double &res_x, long double &res_y) {
    long double c = std::cos(a);
    long double s = std::sin(a);

//     long double x = v_x;
//     long double y = v_y;

    res_x = x * c - y * s;
    res_y = x * s + y * c;
  }

//   highp_vec2 rotate(highp_vec2 v, long double a) {
//     long double c = std::cos(a);
//     long double s = std::sin(a);
//     
//     long double x = v.x;
//     long double y = v.y;
// 
//     highp_vec2 res;
//     res.x = x * c - y * s; 
//     res.y = x * s + y * c;
// 
//     return res;
//   }


  dvec2 calcDiff(dvec2 pos, double next_x) {

//     cout<<sizeof(long double)<<endl;
//     exit(0);
#if 1
//     const double error_margind = 100.0;
//     const long double error_margin = error_margind;

    assert(pos == pos);

    pos.y = 0.0;

    pos.y += planet_radius;

    
   
    if (pos.x == 0.0) {
      assert(pos.y > 0.0);
//       return pos;
      return dvec2(0);
    }

//     double new_y = sqrt(pos.y*pos.y - pos.x*pos.x);
//     return dvec2(pos.x, new_y);
    
    long double horizontalDist = pos.x;
    
//     Complex128 angle = ((long double)2.0 * PI * horizontalDist) / circumference;

    long double angle = ((long double)2.0 * PI * horizontalDist) / circumference;

    long double new_pos_x;
    long double new_pos_y;
    rotate(0, pos.y, -angle, new_pos_x, new_pos_y);
    
//     cout<<pos.x / (new_pos_y - planet_radius)<<endl;
//     cout<<(pos.y-planet_radius)/-new_pos_y+planet_radius<<endl;
    
    
//     return dvec2(pos.x / new_pos_x, pos.x / (new_pos_y - planet_radius));
//     return dvec2(pos.x / new_pos_x, (new_pos_y - planet_radius) / (long double)next_x);
    
    dvec2 new_pos(new_pos_x, new_pos_y);
//       highp_vec2 new_pos = rotate(highp_vec2(0, pos.y), -angle);
    
    assert(new_pos == new_pos);

    assert(new_pos != dvec2(0,0));

//     cout<<new_pos.x<<':'<<new_pos.y<<endl;
    
//     long double pos_x = pos.x;

//     if (!lessOrEqual(new_pos_x, pos_x, error_margin)) {
// //       printf("PI: %.60Lf\n", PI);
//       cout.precision(160);
// //       cout<<"sizeof(long double):"<<sizeof(long double)<<endl;
// //       cout<<"PI:"<<PI<<endl;
//       cout<<"angle:"<<angle<<endl;
//       cout<<"cos:"<<std::cos(angle)<<endl;
//       cout<<"sin:"<<std::sin(angle)<<endl;
//       cout<<pos.x<<" "<<new_pos_x<<endl;
//       cout<<pos.y-planet_radius<<" "<<new_pos_y-planet_radius<<endl;
//     }
    
//     assert(lessOrEqual(new_pos.x, pos.x, error_margind));
//     assert(lessOrEqual(new_pos.y, pos.y, error_margind));

    
//     double base_height_diff = planet_radius - dist_height.y;
    
//     horizontalDist = dist_height.x;
    
//     assert(horizontalDist == horizontalDist);
//     assert(base_height_diff == base_height_diff);
//     assert(rot_z == rot_z);

//     return dvec2(rot_z * dvec2(0, horizontalDist), -base_height_diff);

//     cout<<"x:"<<new_pos.x<<endl;
//     cout<<"y:"<<new_pos.y<<endl;

    return -(pos - new_pos);
//     return new_pos / pos;
//       return pos / (new_pos - dvec2(0, planet_radius));
//       return (pos - dvec2(0.0, planet_radius)) / (new_pos*dvec2(1,-1) - dvec2(0.0, planet_radius));
//       return (new_pos - dvec2(0, planet_radius)) / (pos - dvec2(0, planet_radius));
    
//       return new_pos;

#else
    return pos;
#endif
  }

//   void calcCurvatureValues(int start_index, int end_index) {
  void calcCurvatureValues() {
    const int start_index = 0;
    const int end_index = curvature_map_width;

    for (int x = start_index; x < end_index; x++) {
      double x_coord = ((double)x / map_width) * max_distance;
      double next_x_coord = ((double)(x+1) / map_width) * max_distance;
//       cout<<"x:"<<x_coord<<endl;
      assert (x_coord < max_distance);

      for (int y = 0; y < curvature_map_height; y++) {
        const double y_coord = ((double)y / map_height)  * max_elevation;
        assert (y_coord < max_distance);

        glm::dvec2 pos(x_coord, y_coord);
//         glm::dvec2 pos(x_coord, 0);

        assert(pos == pos);
        
//         cout<<"x:"<<pos.x<<endl;
//         cout<<"y:"<<pos.y<<endl;
        

        dvec2 diff = calcDiff(pos, next_x_coord);
//         dvec2 diff = calcDiff(pos, pos.x);

        assert(diff == diff);

//         cout<<"x:"<<diff.x<<endl;
//         cout<<"y:"<<diff.y<<endl;
        
//         assert(diff.y <= 0);
//         assert(diff.x <= 0);

//         map[x][y].x = diff.x;
//         map[x][y].x = diff.0;
        
        map[y][x].x = diff.x;
//         map[y][x].x = 0;
        
        map[y][x].y = diff.y;
        
        
//         map[x][y].y = -100000;
      }

    }
  }

}

int main (int argc, char **argv) {
  if (argc != 2)
    return 1;
  const char *output_path = argv[1];
  
  cout.precision(10);

  calcCurvatureValues();
  
  ofstream out(output_path);

  if (!out.good()) {
    cerr<<"can't open output file "<<output_path<<endl;
    return 1;
  }

  out.write((const char*) map, size_bytes);
  
  if (!out.good()) {
    cerr<<"error during writong to output file "<<output_path<<endl;
    return 1;
  }

  return 0;
}
