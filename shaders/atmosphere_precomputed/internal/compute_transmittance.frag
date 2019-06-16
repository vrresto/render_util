#version 330

#include definitions.glsl
#include constants.glsl

layout(location = 0) out vec3 transmittance;

void main()
{
  transmittance = 
      ComputeTransmittanceToTopAtmosphereBoundaryTexture(ATMOSPHERE, gl_FragCoord.xy);
}
