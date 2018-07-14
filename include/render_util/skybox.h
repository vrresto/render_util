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

#ifndef RENDER_UTIL_SKYBOX_H
#define RENDER_UTIL_SKYBOX_H

namespace render_util
{

  inline void drawSkyBox()
  {
    const int x = 0;
    const int y = 0;
    const int z = 0;
    const int size = 100000;

    // top
    gl::Begin(GL_POLYGON);
    gl::Vertex3f(  size+x, -size+y, size+z );
    gl::Vertex3f(  size+x,  size+y, size+z );
    gl::Vertex3f( -size+x,  size+y, size+z );
    gl::Vertex3f( -size+x, -size+y, size+z );
    gl::End();

    // bottom
    gl::Begin(GL_POLYGON);
    gl::Vertex3f( -size+x, -size+y, -size+z );
    gl::Vertex3f( -size+x,  size+y, -size+z );
    gl::Vertex3f(  size+x,  size+y, -size+z );
    gl::Vertex3f(  size+x, -size+y, -size+z );
    gl::End();

    // east
    gl::Begin(GL_POLYGON);
    gl::Vertex3f( size+x, -size+y, -size+z );
    gl::Vertex3f( size+x,  size+y, -size+z );
    gl::Vertex3f( size+x,  size+y,  size+z );
    gl::Vertex3f( size+x, -size+y,  size+z );
    gl::End();
  
    // west
    gl::Begin(GL_POLYGON);
    gl::Vertex3f( -size+x,  size+y,  size+z );
    gl::Vertex3f( -size+x,  size+y, -size+z );
    gl::Vertex3f( -size+x, -size+y, -size+z );    
    gl::Vertex3f( -size+x, -size+y,  size+z );
    gl::End();
  
    // north
    gl::Begin(GL_POLYGON);
    gl::Vertex3f(  size+x,  size+y,  size+z );
    gl::Vertex3f(  size+x,  size+y, -size+z );
    gl::Vertex3f( -size+x,  size+y, -size+z );
    gl::Vertex3f( -size+x,  size+y,  size+z );
    gl::End();
  
    // south
    gl::Begin(GL_POLYGON);
    gl::Vertex3f(  size+x, -size+y, -size+z );
    gl::Vertex3f(  size+x, -size+y,  size+z );
    gl::Vertex3f( -size+x, -size+y,  size+z );
    gl::Vertex3f( -size+x, -size+y, -size+z );
    gl::End();
  }

}

#endif
