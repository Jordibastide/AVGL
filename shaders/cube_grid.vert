#version 410 core

#define POSITION	0
#define NORMAL		1
#define TEXCOORD	2
#define FRAG_COLOR	0

precision highp float;
precision highp int;

uniform mat4 MV;
uniform mat4 MVP;
uniform vec3 camPos;
uniform int grid_size;
uniform float time;

layout(location = POSITION) in vec3 Position;
layout(location = NORMAL) in vec3 Normal;
layout(location = TEXCOORD) in vec2 Texcoord;

out block
{
	vec2 Texcoord;
	vec3 CameraSpacePosition;
    vec3 CameraSpaceNormal;
    vec3 wPosition;

} Out;

vec3 mod289(vec3 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec2 mod289(vec2 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec3 permute(vec3 x) {
  return mod289(((x*34.0)+1.0)*x);
}

float snoise(vec2 v)
  {
  const vec4 C = vec4(0.211324865405187,
                      0.366025403784439,
                     -0.577350269189626,
                      0.024390243902439);
// First corner
  vec2 i  = floor(v + dot(v, C.yy) );
  vec2 x0 = v -   i + dot(i, C.xx);

// Other corners
  vec2 i1;
  i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
  vec4 x12 = x0.xyxy + C.xxzz;
  x12.xy -= i1;

// Permutations
  i = mod289(i); // Avoid truncation effects in permutation
  vec3 p = permute( permute( i.y + vec3(0.0, i1.y, 1.0 ))
		+ i.x + vec3(0.0, i1.x, 1.0 ));

  vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy), dot(x12.zw,x12.zw)), 0.0);
  m = m*m ;
  m = m*m ;

// Gradients: 41 points uniformly over a line, mapped onto a diamond.

  vec3 x = 2.0 * fract(p * C.www) - 1.0;
  vec3 h = abs(x) - 0.5;
  vec3 ox = floor(x + 0.5);
  vec3 a0 = x - ox;

// Normalise gradients implicitly by scaling m
// Approximation of: m *= inversesqrt( a0*a0 + h*h );
  m *= 1.79284291400159 - 0.85373472095314 * ( a0*a0 + h*h );

// Compute final noise value at P
  vec3 g;
  g.x  = a0.x  * x0.x  + h.x  * x0.y;
  g.yz = a0.yz * x12.xz + h.yz * x12.yw;
  return 130.0 * dot(m, g);
}


void main()
{
    vec3 p = Position;
    float camDist = 15.f;

    float translateX = (gl_InstanceID % grid_size) - (grid_size / 2);
    float translateZ = (gl_InstanceID / grid_size) - (grid_size / 2);

    p.x += translateX;
    p.z += translateZ;

    float noise = snoise(vec2((gl_InstanceID % grid_size) - (grid_size / 2) * time * 0.001, (gl_InstanceID / grid_size) - (grid_size / 2)));
    if (noise < 0.f) {noise = - noise;}

    if (distance(vec3(translateX * 2, 10.f * noise, translateZ * 2), camPos) > camDist){

        p.y *= 10.f * noise;
    }

    p.x *= 2;
    p.z *= 2;

    Out.Texcoord = Texcoord;
    Out.CameraSpacePosition = vec3(MV * vec4(p, 1.0));
    Out.CameraSpaceNormal = vec3(MV * vec4(Normal, 0.0));
    Out.wPosition = p;

	gl_Position = MVP * vec4(p, 1.0);
}