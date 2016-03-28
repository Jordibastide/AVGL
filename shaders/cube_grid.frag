#version 410 core

#define POSITION	0
#define NORMAL		1
#define TEXCOORD	2
#define FRAG_COLOR	0

precision highp int;

uniform vec3 colorNear;
uniform vec3 colorFar;
uniform int grid_size;
uniform float brightness;
uniform float attenuation;
uniform mat4 MV;
uniform vec3 lightDir;
uniform vec3 camPos;

layout(location = FRAG_COLOR, index = 0) out vec4 FragColor;
layout(location = NORMAL) out vec4 Normal;

const float b = 0.01;

in block
{
	vec2 Texcoord;
	vec3 CameraSpacePosition;
    vec3 CameraSpaceNormal;
    vec3 wPosition;
} In;

vec3 applyFog(vec3  rgb, float distance, vec3  rayOri, vec3  rayDir )
{
    float fogAmount = 1.5 * exp(-rayOri.y*b) * (1.0-exp( -distance*rayDir.y*b ))/rayDir.y;
    vec3  fogColor  = vec3(0.f);
    return mix( rgb, fogColor, fogAmount );
}

vec3 getColor()
{
    float halfGrid = grid_size * 0.5;
    float ratio = distance(vec3(-halfGrid, 5, -halfGrid), In.wPosition) / grid_size;
    ratio = max(min(ratio, 1.0), 0.0);
    return colorFar * ratio + colorNear * (1.0 - ratio);
}

void main()
{
    vec2 multiplier = pow( abs( In.Texcoord - 0.5 ), vec2( attenuation ) );

    vec3 colorShaded = getColor() * brightness * length( multiplier );
    float camToPointDist = distance(In.wPosition, camPos);
    vec3 rayDir = normalize(In.wPosition - camPos);
    vec3 sunDir = vec3(MV * vec4(lightDir, 0.f));

    vec3  diffuseColor = vec3(0.5f);
    float specularColor = 0.5f;

    FragColor = vec4(applyFog(colorShaded, camToPointDist, camPos, rayDir) , specularColor);
    Normal = vec4( normalize(In.CameraSpaceNormal), 15.f);
}
