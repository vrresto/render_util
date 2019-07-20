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

/**
 * This program uses methods described in the following papers:
 *
 * "Real-Time Rendering of Planets with Atmospheres" by Tobias Schafhitzel, Martin Falk,  Thomas Ertl
 * <https://www.researchgate.net/publication/230708134_Real-Time_Rendering_of_Planets_with_Atmospheres>
 *
 * "Precomputed Atmospheric Scattering" by Eric Bruneton, and Fabrice Neyret
 * <https://hal.inria.fr/inria-00288758/en>
 */

#include <dispatcher.h>
#include <distances.h>
#include <util.h>
#include <atmosphere_map.h>
#include <render_util/render_util.h>
#include <render_util/physics.h>
#include <render_util/image.h>
#include <render_util/image_loader.h>
#include <render_util/image_util.h>

#include <iostream>
#include <fstream>

using namespace std;

namespace
{

//   void printFloat128(const _Float128 &value)
//   {
//     char buffer[1000];
//     strfromf128(buffer, sizeof(buffer), "%.5f", value);
//     cout<<buffer;
//   }

  constexpr auto planet_radius = render_util::physics::EARTH_RADIUS;
  const double max_atmosphere_distance = 1800.0 * 1000.0; //FIXME calculate the correct value

  const glm::ivec2 &map_size = atmosphere_map_dimensions;
  const int map_num_rows = map_size.y;
  const int map_num_elements = ATMOSPHERE_MAP_NUM_ELEMENTS;

  AtmosphereMapElementType g_map[map_num_elements];


  double getDistanceToHorizon(double r)
  {
    if (r <= planet_radius)
      return 0.0;
    else
      return sqrt(r*r - planet_radius*planet_radius);
  }

  double getMaxAtmosphereDistanceAtHeight(double height)
  {
    return getDistanceToHorizon(planet_radius + height) +
           getDistanceToHorizon(planet_radius + atmosphere_height);
  }

  double calcHazeDensityAtHeight(double height)
  {
    return exp(-(height/1300));
  }

  double calcAtmosphereDensityAtHeight(double height)
  {
    assert(height >= 0);

    // using the Earth Atmosphere Model as described in
    // https://www.grc.nasa.gov/www/k-12/airplane/atmosmet.html

    double t = 0; // temperature
    double p = 0; // pressure

    if (height < 11000) {
      t = 15.04 - 0.00649 * height;
      p = 101.29 * pow((t + 273.1) / 288.08, 5.256);
    }
    else if (height < 25000) {
      t = -56.46;
      p = 22.65 * exp(1.73 - 0.000157 * height);
    }
    else {
      t = -131.21 + 0.00299 * height;
      p = 2.488 * pow((t + 273.1) / 216.6, -11.388);
    }

    double density = p / (0.2869 * (t + 273.1));

    return density;
  }

  const double max_air_density = calcAtmosphereDensityAtHeight(0);

  const double fog_layer_top = 400.0;

  double calcFogDensityAtHeight(double height)
  {
    return 0;
//     return height < fog_layer_top ? 1.0 : 0.0;
//     return exp(-5 * (height / fog_layer_top));
  }

  double calcRelativeAtmosphereDensityAtHeight(double height)
  {
#if 1

//     {
//       unique_lock lock(start_cond_mutex);
//
//       assert(!isnan(height));
//
//       if (height < 0.00)
//       {
//         cout<<height<<endl;
//       }
//
//       assert(height >= 0);
//     }

    assert(!isnan(height));
    assert(height >= 0);


    double density = calcAtmosphereDensityAtHeight(height);
    density = min(density, max_air_density);
    assert(density <= max_air_density);

    double relative_density = density / max_air_density;

    if (!(relative_density <= 1.0))
    {
      cout<<"height: "<<height<<endl;
      cout<<"density: "<<density<<endl;
      cout<<"max_air_density: "<<max_air_density<<endl;
      cout<<"relative_density: "<<relative_density<<endl;
    }

    assert(relative_density >= 0.0);
    assert(relative_density <= 1.0);

//     relative_density = 0;
//     relative_density += 100 * calcFogDensityAtHeight(height);

    return relative_density;
#else
    if (height < 20000.0)
      return 1;
    else
      return 0;
#endif
  }

  glm::dvec2 calcAtmosphereThickness(double camera_height, const glm::dvec2 &view_dir)
  {
    const double sampling_step = 100.0;
    const int max_steps = max_atmosphere_distance / sampling_step;

    assert(!isnan(max_steps));
    assert(max_steps > 1);

    const glm::dvec2 camera_pos(0, camera_height);

    double total = 0;
    double haze_total = 0;
    int walked_steps = 0;

    //FIXME calculate exact distance to surface

    for (int i = 0; i < max_steps; i++) {
      const double sample_dist = i * sampling_step;

// earth curvature
#if 1
      const glm::dvec2 pos = glm::dvec2(0, planet_radius) + camera_pos + (view_dir * sample_dist);
//       pos.y += planet_radius + camera_height;
      const double height = glm::length(pos) - planet_radius;
#else
      const glm::dvec2 pos = camera_pos + (view_dir * sample_dist);
      const double height = pos.y;
#endif

      assert(!isnan(height));


      if (height < 0.0)
        break;

      const double density_at_step = calcRelativeAtmosphereDensityAtHeight(height);
      const double haze_density_at_step = calcHazeDensityAtHeight(height);

      walked_steps++;
      total += density_at_step;
      haze_total += haze_density_at_step;

      assert(total == total);
    }

//     double average = total / walked_steps;
//     assert(!isnan(average));
//     return walked_steps * sampling_step * average;
    // <=>
    return sampling_step * glm::dvec2(total, haze_total);
  }

  int getMapIndex(int x, int y) {
    //const int index = (z * map_size.y * map_size.x) + (y * map_size.x) + x;
    //assert(index < (map_size.x * map_size.y * map_size.z));

    const int index = (y * map_size.x) + x;
    assert(index < map_num_elements);

    return index;
  }

  void calcAtmosphereDensityValues(int y)
  {
    const double camera_height_step = atmosphere_height / (double) map_size.y;
    const double view_dir_step = 1.0 / (double) map_size.x;

    float Rg = planet_radius;
    float Rt = planet_radius + atmosphere_height;
    float H = pow(Rt*Rt - Rg*Rg, 0.5);
    float Ur = (float)y / (float)map_size.y;
    float p = Ur * H;

    float r = sqrt(pow(p, 2) + Rg*Rg);

//       cout<<"r(km): "<<r/1000.0<<endl;

    const double camera_height = r - planet_radius;
//       const double camera_height = y * camera_height_step;
    const double max_atmosphere_distance_at_camera_height = getMaxAtmosphereDistanceAtHeight(camera_height);
    const double min_atmosphere_distance_at_camera_height = atmosphere_height - camera_height;

//       cout<<"camera_height(km): "<<camera_height/1000.0<<endl;

    assert(camera_height >= 0.0);

    for (int x = 0; x < map_size.x; x++)
    {
      glm::dvec2 thickness = glm::dvec2(-1);

      const double x_normalized = x * view_dir_step;

      assert(!isnan(x_normalized));
      assert(!isnan(max_atmosphere_distance_at_camera_height));


      const double min_mu = min_atmosphere_distance_at_camera_height / max_atmosphere_distance_at_camera_height;
      const double max_mu = 1.0;

      double mu = (x_normalized * (max_mu - min_mu)) + min_mu;

      mu *= max_atmosphere_distance_at_camera_height;

      double atmosphere_distance_in_view_direction = mu;

//         if (!(atmosphere_distance_in_view_direction+1 >= min_atmosphere_distance_at_camera_height)) {
//           unique_lock lock(start_cond_mutex);
//           cout<<"atmosphere_distance_in_view_direction: "<<atmosphere_distance_in_view_direction<<endl;
//           cout<<"min_atmosphere_distance_at_camera_height: "<<min_atmosphere_distance_at_camera_height<<endl;
//           exit(1);
//         }

      assert(atmosphere_distance_in_view_direction+1 >= min_atmosphere_distance_at_camera_height);

      // clamping
      atmosphere_distance_in_view_direction =
        max(min_atmosphere_distance_at_camera_height, atmosphere_distance_in_view_direction);

      assert(atmosphere_distance_in_view_direction >= min_atmosphere_distance_at_camera_height);
      assert(atmosphere_distance_in_view_direction <= max_atmosphere_distance_at_camera_height);


//         if (x != 0 && atmosphere_distance_in_view_direction >= min_atmosphere_distance_at_camera_height)
      if (1)
      {
//           const double x_normalized = 0.99;


        assert(!isnan(atmosphere_distance_in_view_direction));

        if (atmosphere_distance_in_view_direction <= 0.0) {
          atmosphere_distance_in_view_direction = 0.001;
        }


        assert(atmosphere_distance_in_view_direction > 0.0);

        const long double a = atmosphere_distance_in_view_direction;
        const long double b = planet_radius + camera_height;
        const long double c = planet_radius + atmosphere_height;


        assert(a > 0.0);
        assert(b > 0.0);
        assert(c > 0.0);

//           cout<<"a:"<<(long double)a<<endl;
//           cout<<"b:"<<(long double)b<<endl;
//           cout<<"c:"<<(long double)c<<endl;
//           const long double cos_view_dir = ((a*a + b*b) - c*c) / (((long double)2.0)*a*b);
        long double cos_view_dir = 0;
        {
          long double z = ((a*a + b*b) - c*c);
          long double n = (((long double)2.0)*a*b);

//             cout<<"z:"<<(long double)z<<endl;
//             cout<<"n:"<<(long double)n<<endl;

          cos_view_dir =  z / n;

//             unique_lock lock(start_cond_mutex);
//             cout<<"z: "; printFloat128(z); cout<<endl;
//             cout<<"n: "; printFloat128(n); cout<<endl;
        }

//           {
//             char buffer[1000];
//
//             strfromf128(buffer, sizeof(buffer), "%.5f", cos_view_dir);
//
//             unique_lock lock(start_cond_mutex);
// //             cout<<"cos_view_dir: "<<buffer<<endl;
// //             cout<<cos_view_dir<<endl;
//           }


        assert(cos_view_dir >= -1.0);
        assert(cos_view_dir <= 1.0);

//           const double view_dir_y = x_normalized * 2 - 1.0;
//           const double view_dir_x = glm::sqrt(1.0 - view_dir_y * view_dir_y);
        const double view_dir_x = cos_view_dir;
        const double view_dir_y = glm::sqrt(1.0 - view_dir_x*view_dir_x);

        assert(!isnan(view_dir_x));
        assert(!isnan(view_dir_y));

//           const glm::dvec2 view_dir(view_dir_x, view_dir_y);
        const glm::dvec2 view_dir(view_dir_y, -view_dir_x);

        thickness = calcAtmosphereThickness(camera_height, view_dir);
//           thickness = calcAtmosphereThickness(0, glm::normalize(glm::vec2(1.0)));
//           thickness = cos_view_dir;
      }
      else {
//           thickness = -1.0 - x_normalized;
        thickness = glm::dvec2(-1.0);
      }

      g_map[getMapIndex(x,y)] = { (float)thickness.x, (float)thickness.y };

    }

  }


  constexpr float ATMOSPHERE_VISIBILITY = 600000.0;

  float getTransmittance(float dist)
  {
    return exp(-3.0 * (dist / ATMOSPHERE_VISIBILITY));
  }
  

} // namespace


bool render_util::createAtmosphereMap(const char *output_path)
{
  Dispatcher dispatcher(calcAtmosphereDensityValues);

  dispatcher.dispatch(map_num_rows);
  
  auto image = make_shared<render_util::ImageGreyScale>(map_size);
  
  
  for (int y = 0; y < map_size.y; y++)
  {
    for (int x = 0; x < map_size.x; x++)
    {
      auto index = getMapIndex(x,y);
      auto transmittance = getTransmittance(g_map[index].air_thickness);

      image->at(x,y) = 255 * (1.f - transmittance);
    }
  }
  
  image = image::flipY(image);
  
  render_util::saveImageToFile("/tmp/atmosphere_map.png", image.get(), render_util::ImageType::PNG);

  ofstream out(output_path);
  
  return util::writeFile(output_path, (const char*) g_map, sizeof(g_map));
}
