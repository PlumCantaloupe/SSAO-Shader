#version 120

varying vec3 Normal;
varying float depth; //in eye space

void main( void )
{
	vec4 viewPos = gl_ModelViewMatrix * gl_Vertex;
	depth = -viewPos.z/10.0;

	gl_Position = ftransform();
	Normal      = normalize(( gl_ModelViewMatrix * vec4( gl_Normal.xyz, 0.0 ) ).xyz);
}