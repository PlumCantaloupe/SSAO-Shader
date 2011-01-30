#version 120

varying vec3 Normal;
varying float depth;

void main( void )
{
   gl_FragColor = vec4(normalize(Normal),depth);
}