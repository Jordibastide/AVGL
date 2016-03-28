#version 410 core

#define POSITION	0
#define NORMAL		1
#define TEXCOORD	2
#define FRAG_COLOR	0

precision highp float;
precision highp int;

layout(location = POSITION) in vec3 Position;
layout(location = NORMAL) in vec3 Normal;
layout(location = TEXCOORD) in vec2 Texcoord;

uniform mat4 MV;
uniform mat4 MVP;

uniform vec3 position_scripted;

out block
{
	vec3 Position;
	vec3 Normal;
} Out;

void main()
{
  Out.Normal = normalize((MV * vec4(Normal, 0.0)).xyz);
  Out.Position = (MV * vec4(Position, 1.0)).xyz;

  vec3 pos = Position * 0.2f;
  pos += position_scripted;
  //pos *= 5.f;
  gl_Position = MVP * vec4(pos, 1.0);
}