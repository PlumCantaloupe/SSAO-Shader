#version 120
//http://www.gamerendering.com/2009/01/14/ssao/

varying vec2  uv;
 
void main(void)
{
	gl_Position = ftransform();
	gl_Position = sign( gl_Position );
 
	//Texture coordinate for screen aligned (in correct range):
	uv = (vec2( gl_Position.x, gl_Position.y ) + vec2( 1.0 ) ) * 0.5;
}