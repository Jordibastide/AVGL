#version 410 core

precision highp float;
precision highp int;

#define FRAG_COLOR	0
#define NORMAL		1

uniform vec3 Color;

const float alpha = 1.0;
const float start = 0.0;
const float end = 0.9;

layout(location = FRAG_COLOR, index = 0) out vec4 FragColor;
layout(location = NORMAL) out vec4 Normal;

in block
{
	vec3 Position;
	vec3 Normal;
} In;

void main() {
    vec3 normal = normalize( In.Normal );
    vec3 eye = normalize( -In.Position.xyz );
    float rim = smoothstep( start, end, 1.0 - dot( normal, eye ) );
    float value = clamp( rim * alpha, 0.0, 1.0 );
    FragColor = vec4( Color, 0.5f );
    Normal = vec4( normalize(In.Normal), 15.f);
}
