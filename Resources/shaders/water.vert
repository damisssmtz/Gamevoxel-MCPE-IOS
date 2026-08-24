#version 300 es
precision highp float;

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;
layout(location = 2) in vec4 a_Color;
layout(location = 3) in vec3 a_Normal;

uniform mat4 u_MVP;
uniform mat4 u_ModelView;
uniform float u_Time;
uniform vec3 u_ChunkPos;

out vec2 v_TexCoord;
out vec4 v_Color;
out float v_FogDepth;
out vec3 v_WorldPos;
out vec3 v_ViewPos;
out vec3 v_WaveNormalView;
out vec3 v_WorldUpView;
out float v_WaveHeight;
out float v_TopFace;

const float WAVE_AMPLITUDE = 0.045;
const float WAVE_SPEED = 0.9;

void main()
{
    v_TexCoord = a_TexCoord;
    v_Color = a_Color;

    float normalY = max(a_Normal.y, 0.0);
    v_TopFace = smoothstep(0.4, 0.75, normalY);

    vec3 worldPos = a_Position + u_ChunkPos;

    float t = u_Time * WAVE_SPEED;

    vec2 d1 = normalize(vec2(1.0, 0.25));
    vec2 d2 = normalize(vec2(-0.4, 1.0));
    vec2 d3 = normalize(vec2(0.8, -0.7));
    vec2 d4 = normalize(vec2(-1.0, -0.2));

    float w1 = sin(dot(worldPos.xz, d1) * 1.7 + t);
    float w2 = sin(dot(worldPos.xz, d2) * 2.4 - t * 1.2);
    float w3 = sin(dot(worldPos.xz, d3) * 3.1 + t * 0.7);
    float w4 = sin(dot(worldPos.xz, d4) * 4.2 - t * 0.5);

    float height =
        w1 * 0.55 +
        w2 * 0.25 +
        w3 * 0.13 +
        w4 * 0.07;

    v_WaveHeight = height * WAVE_AMPLITUDE;

    vec3 pos = a_Position;
    v_WorldPos = worldPos;

    float eps = 0.15;

    float hx =
        (sin(dot((worldPos.xz + vec2(eps,0.0)), d1) * 1.7 + t) * 0.55 +
         sin(dot((worldPos.xz + vec2(eps,0.0)), d2) * 2.4 - t*1.2) * 0.25 +
         sin(dot((worldPos.xz + vec2(eps,0.0)), d3) * 3.1 + t*0.7) * 0.13 +
         sin(dot((worldPos.xz + vec2(eps,0.0)), d4) * 4.2 - t*0.5) * 0.07);

    float hz =
        (sin(dot((worldPos.xz + vec2(0.0,eps)), d1) * 1.7 + t) * 0.55 +
         sin(dot((worldPos.xz + vec2(0.0,eps)), d2) * 2.4 - t*1.2) * 0.25 +
         sin(dot((worldPos.xz + vec2(0.0,eps)), d3) * 3.1 + t*0.7) * 0.13 +
         sin(dot((worldPos.xz + vec2(0.0,eps)), d4) * 4.2 - t*0.5) * 0.07);

    vec3 tangent =
        normalize(vec3(eps, (hx - height) * WAVE_AMPLITUDE, 0.0));

    vec3 binormal =
        normalize(vec3(0.0, (hz - height) * WAVE_AMPLITUDE, eps));

    vec3 normal =
        normalize(cross(binormal, tangent));

    mat3 normalMatrix = mat3(u_ModelView);
    v_WaveNormalView = normalize(normalMatrix * normal);
    v_WorldUpView = normalize(normalMatrix * vec3(0.0, 1.0, 0.0));

    vec4 viewPos = u_ModelView * vec4(pos, 1.0);

    v_ViewPos = viewPos.xyz;
    v_FogDepth = length(viewPos.xyz);

    gl_Position = u_MVP * vec4(pos, 1.0);
}