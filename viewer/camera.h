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

#ifndef CAMERA_H
#define CAMERA_H

#include <render_util/camera.h>

#include <glm/gtc/matrix_transform.hpp>


namespace render_util::viewer
{


struct Camera : public render_util::Camera
{
  float x = 0;
  float y = 0;
  float z = 0;
  float yaw = 0;
  float pitch = 0;
  float roll = 0;

  Camera() {
    updateTransform();
  }

  glm::vec4 getDirection()
  {
    glm::mat4 rot_yaw = glm::rotate(glm::mat4(1), glm::radians(yaw), glm::vec3(0,0,1));
    glm::vec4 pitch_axis(0,1,0,0);
    pitch_axis = rot_yaw * pitch_axis;
    glm::mat4 rot_pitch =
      glm::rotate(glm::mat4(1), glm::radians(-pitch), glm::vec3(pitch_axis.x, pitch_axis.y, pitch_axis.z));

    glm::vec4 direction(1,0,0,0);
    direction = rot_pitch * rot_yaw * direction;

    return direction;
  }
  
  void moveForward(float dist)
  {
//       cout<<"yaw: "<<yaw<<"\tpitch: "<<pitch<<endl;
    
    glm::vec4 direction = getDirection();
    
//       cout<<direction.x<<" "<<direction.y<<" "<<direction.z<<endl;

    glm::vec4 pos(x,y,z,1);

    pos += direction * dist;

    x = pos.x;
    y = pos.y;
    z = pos.z;
  }

  void updateTransform()
  {
//       cout<<"yaw: "<<yaw<<"\tpitch: "<<pitch<<endl;
    setTransform(x, y, z, yaw, pitch, roll);
  }

};


} // namespace render_util::viewer


#endif
