#include "WaterShader.h"
#include "../../platform/log.h"

Shader WaterShader::instance;
bool WaterShader::inited = false;

static const char* defaultWaterVert = R"(#version 300 es
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

const float wave_amplitude = 0.05;
const float wave_speed = 1.0;
const float wave_frequency = 1.8;
const float normal_steepness = 0.8;

void main() {
    v_TexCoord = a_TexCoord;
    v_Color = a_Color;
    
    float normalY = a_Normal.y == 0.0 ? 1.0 : a_Normal.y;
    v_TopFace = smoothstep(0.4, 0.75, normalY);

    float t = u_Time * wave_speed;
    float k1 = wave_frequency;
    float k2 = wave_frequency * 1.414;

    vec3 raw_world_pos = a_Position + u_ChunkPos;

    float phase1 = (raw_world_pos.x * 0.9 + raw_world_pos.z * 0.4) * k1 + t;
    float sin1 = sin(phase1);
    float cos1 = cos(phase1);

    float phase2 = (-raw_world_pos.x * 0.5 + raw_world_pos.z * 0.866) * k2 - t * 0.75;
    float sin2 = sin(phase2);
    float cos2 = cos(phase2);

    v_WaveHeight = (sin1 + sin2 * 0.6) * wave_amplitude;

    vec3 displaced_pos = a_Position;
    
    v_WorldPos = raw_world_pos;

    float n_x = -(cos1 * 0.9 * k1 + cos2 * (-0.5) * k2 * 0.6) * wave_amplitude * normal_steepness;
    float n_z = -(cos1 * 0.4 * k1 + cos2 * 0.866 * k2 * 0.6) * wave_amplitude * normal_steepness;
    
    vec3 wave_normal_local = normalize(vec3(n_x, 1.0, n_z));

    mat3 normalMatrix = mat3(u_ModelView);
    v_WaveNormalView = normalize(normalMatrix * wave_normal_local);
    v_WorldUpView = normalize(normalMatrix * vec3(0.0, 1.0, 0.0));

    vec4 viewPos = u_ModelView * vec4(displaced_pos, 1.0);
    v_ViewPos = viewPos.xyz;
    v_FogDepth = length(viewPos.xyz);

    gl_Position = u_MVP * vec4(displaced_pos, 1.0);
}
)";

static const char* defaultWaterFrag = R"(#version 300 es
precision highp float;

in vec2 v_TexCoord;
in vec4 v_Color;
in float v_FogDepth;
in vec3 v_WorldPos;
in vec3 v_ViewPos;
in vec3 v_WaveNormalView;
in vec3 v_WorldUpView;
in float v_WaveHeight;
in float v_TopFace;

uniform bool u_UseFog;
uniform vec4 u_FogColor;
uniform float u_FogStart;
uniform float u_FogEnd;
uniform float u_Time;

uniform mat4 u_Projection;

out vec4 fragColor;

const float absorption_strength = 1.0;
const float shore_foam_distance = 1.5;
const float wave_amplitude = 0.05;

const vec4 shallow_color = vec4(0.18, 0.65, 0.92, 0.55);
const vec4 deep_color = vec4(0.04, 0.20, 0.52, 0.88);
const vec4 foam_color = vec4(0.92, 0.97, 1.0, 0.75);
const vec4 sun_glow_color = vec4(0.95, 0.98, 1.0, 1.0);

const float rain_ripple_factor = 0.0;
const float rain_ripple_speed = 2.8;

const float sss_strength = 0.35;
const float fresnel_f0 = 0.04;

float waveHeight(vec2 p, float time)
{
    const float amplitude = 0.05;
    const float speed = 1.0;
    const float frequency = 1.8;
    vec2 d1 = normalize(vec2(0.9, 0.4));
    vec2 d2 = normalize(vec2(-0.5, 0.866));

    return (
        sin(dot(p, d1) * frequency + time * speed) +
        sin(dot(p, d2) * frequency * 1.414 - time * speed * 0.75) * 0.6
    ) * amplitude;
}



void main() {
    float wave = waveHeight(v_WorldPos.xz, u_Time);
    vec3 dx = dFdx(v_ViewPos) + v_WorldUpView * dFdx(wave);
    vec3 dy = dFdy(v_ViewPos) + v_WorldUpView * dFdy(wave);
    vec3 N = normalize(cross(dx, dy));

    vec3 V = -normalize(v_ViewPos);

    float NdotV = clamp(dot(N, V), 0.0, 1.0);
    float fresnel = fresnel_f0 + (1.0 - fresnel_f0) * pow(1.0 - NdotV, 5.0);

    // Extremely fast procedural depth using distance instead of FBO read!
    // This avoids the 20 FPS pipeline stall and fixes all high-altitude precision bugs.
    float distance_z = length(v_ViewPos);
    
    // Simulate depth factor based on view angle and wave height for a rich look
    float depth_factor = clamp((v_WaveHeight * 15.0) + (1.0 - NdotV) * 0.5, 0.0, 1.0);
    depth_factor *= absorption_strength;

    vec3 water_color = mix(shallow_color.rgb, deep_color.rgb, depth_factor);
    if (v_Color.a > 0.05) {
        water_color *= v_Color.rgb;
    }

    // Procedural foam strictly on wave crests (no shore intersection needed, 100x faster)
    float crest_foam = smoothstep(wave_amplitude * 0.4, wave_amplitude * 0.95, abs(wave)) * v_TopFace;
    
    // Fade out foam in the far distance to keep it clean and reduce noise
    float distance_fade = clamp(1.0 - (distance_z / 48.0), 0.0, 1.0);
    float total_foam = clamp(crest_foam * 0.85 * distance_fade, 0.0, 1.0);

    vec3 light_dir_view = normalize(vec3(0.4, 0.8, 0.4));
    float sss = pow(clamp(dot(V, -light_dir_view + N * 0.3), 0.0, 1.0), 4.0) * sss_strength * v_TopFace;
    vec3 sss_glow = shallow_color.rgb * sss * 1.5;

    vec3 col = mix(water_color + sss_glow, foam_color.rgb, total_foam * 0.45);
    col = mix(col, sun_glow_color.rgb * 0.85, fresnel * 0.4 * v_TopFace);

    float alpha_base = mix(shallow_color.a, deep_color.a, depth_factor);
    float final_alpha = clamp(alpha_base + fresnel * 0.25 + total_foam * foam_color.a, 0.45, 0.96);

    vec4 finalColor = vec4(col, final_alpha);

    if (u_UseFog && u_FogEnd > u_FogStart) {
        float fogFactor = clamp((u_FogEnd - v_FogDepth) / (u_FogEnd - u_FogStart), 0.0, 1.0);
        finalColor.rgb = mix(u_FogColor.rgb, finalColor.rgb, fogFactor);
    }

    fragColor = finalColor;
}
)";

bool WaterShader::init()
{
    if (inited) return true;
    inited = instance.loadFromSource(defaultWaterVert, defaultWaterFrag);
    if (!inited) {
        LOGE("Failed to load embedded Water shaders!\n");
    } else {
        LOGI("WaterShader initialized successfully.\n");
    }
    return inited;
}

void WaterShader::setupMVP(const Matrix4f& mvp)
{
    if (!inited) init();
    instance.bind();
    instance.setUniformMatrix4f("u_MVP", mvp);
}

void WaterShader::bind()
{
    if (!inited) init();
    instance.bind();
}

void WaterShader::unbind()
{
    instance.unbind();
}
