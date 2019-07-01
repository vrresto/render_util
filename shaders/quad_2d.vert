#version 130

void main()
{
  gl_Position = vec4(gl_Vertex.x, gl_Vertex.y, 1, 1);
}
