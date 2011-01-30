#version 120
//http://www.gamerendering.com/2009/01/14/ssao/

uniform sampler2D rnm;
uniform sampler2D normalMap;
varying vec2 uv;
const float totStrength = 1.38;
const float strength = 0.07;
const float offset = 18.0;
const float falloff = 0.000002;
const float rad = 0.006;

#define SAMPLES 10 // 10 is good
const float invSamples = 1.0/16.0;
void main(void)
{
	// these are the random vectors inside a unit sphere ( must be defined this way as Apple doesn't support anything beyond GLSL 120 :( FFFFFFUUUUUUUUUUUUUU
	vec3 pSphere[16];
	pSphere[0] = vec3(0.53812504, 0.18565957, -0.43192);
	pSphere[1] = vec3(0.13790712, 0.24864247, 0.44301823);
	pSphere[2] = vec3(0.33715037, 0.56794053, -0.005789503);
	pSphere[3] = vec3(-0.6999805, -0.04511441, -0.0019965635);
	pSphere[4] = vec3(0.06896307, -0.15983082, -0.85477847);
	pSphere[5] = vec3(0.056099437, 0.006954967, -0.1843352);
	pSphere[6] = vec3(-0.014653638, 0.14027752, 0.0762037);
	pSphere[7] = vec3(0.010019933, -0.1924225, -0.034443386);
	pSphere[8] = vec3(-0.35775623, -0.5301969, -0.43581226);
	pSphere[9] = vec3(-0.3169221, 0.106360726, 0.015860917);
	pSphere[10] = vec3(0.010350345, -0.58698344, 0.0046293875);
	pSphere[11] = vec3(-0.08972908, -0.49408212, 0.3287904);
	pSphere[12] = vec3(0.7119986, -0.0154690035, -0.09183723);
	pSphere[13] = vec3(-0.053382345, 0.059675813, -0.5411899);
	pSphere[14] = vec3(0.035267662, -0.063188605, 0.54602677);
	pSphere[15] = vec3(-0.47761092, 0.2847911, -0.0271716);

   // grab a normal for reflecting the sample rays later on
   vec3 fres = normalize((texture2D(rnm,uv*offset).xyz*2.0) - vec3(1.0));
 
   vec4 currentPixelSample = texture2D(normalMap,uv);
 
   float currentPixelDepth = currentPixelSample.a;
 
   // current fragment coords in screen space
   vec3 ep = vec3(uv.xy,currentPixelDepth);
   
	// get the normal of current fragment
   vec3 norm = currentPixelSample.xyz;
 
   float bl = 0.0;
   // adjust for the depth ( not shure if this is good..)
   float radD = rad/currentPixelDepth;
 
   vec3 ray, se, occNorm;
   float occluderDepth, depthDifference, normDiff;
 
   for(int i=0; i < SAMPLES;++i)
   {
		// get a vector (randomized inside of a sphere with radius 1.0) from a texture and reflect it
		ray = radD*reflect(pSphere[i],fres);
		//ray = radD*reflect(vec3(pSphere0[i], pSphere1[i], pSphere2[i]),fres); 

		// if the ray is outside the hemisphere then change direction
		se = ep + sign(dot(ray,norm) )*ray;
 
		// get the depth of the occluder fragment
		vec4 occluderFragment = texture2D(normalMap,se.xy);
 
		// get the normal of the occluder fragment
		occNorm = occluderFragment.xyz;
 
		// if depthDifference is negative = occluder is behind current fragment
		depthDifference = currentPixelDepth-occluderFragment.a;
 
		// calculate the difference between the normals as a weight
 
		normDiff = (1.0-dot(occNorm,norm));
		// the falloff equation, starts at falloff and is kind of 1/x^2 falling
		bl += step(falloff,depthDifference)*normDiff*(1.0-smoothstep(falloff,strength,depthDifference));
   }
 
   // output the result
   float ao = 1.0-totStrength*bl*invSamples;
   gl_FragColor.r = ao;
 
}