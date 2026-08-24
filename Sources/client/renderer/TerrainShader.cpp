#include "TerrainShader.h"
#include "../../platform/log.h"

Shader TerrainShader::instance;
bool TerrainShader::inited = false;

static const char* defaultTerrainVert = R"(#version 300 es
precision highp float;

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;
layout(location = 2) in vec4 a_Color;
layout(location = 3) in vec3 a_Normal;

uniform mat4 u_MVP;
uniform mat4 u_ModelView;

out vec2 v_TexCoord;
out vec4 v_Color;
out float v_FogDepth;
out vec3 v_WorldPos;

void main() {
    v_TexCoord = a_TexCoord;
    v_Color = a_Color;
    v_WorldPos = a_Position;
    
    vec4 viewPos = u_ModelView * vec4(a_Position, 1.0);
    v_FogDepth = length(viewPos.xyz);
    
    gl_Position = u_MVP * vec4(a_Position, 1.0);
}
)";

static const char* defaultTerrainFrag = R"(#version 300 es
precision highp float;

in vec2 v_TexCoord;
in vec4 v_Color;
in float v_FogDepth;
in vec3 v_WorldPos;

uniform sampler2D u_Texture;
uniform bool u_UseTexture;
uniform bool u_UseFog;
uniform bool u_AlphaTest;
uniform int u_IsSky;
uniform vec4 u_FogColor;
uniform float u_FogStart;
uniform float u_FogEnd;

out vec4 fragColor;

void main() {
    vec4 texColor = u_UseTexture ? texture(u_Texture, v_TexCoord) : vec4(1.0);
    vec4 finalColor = texColor * v_Color;

    if (u_IsSky == 1) {
        // Calculate sky gradient based on normalized height (Y axis)
        float h = normalize(v_WorldPos).y;
        
        // Ensure horizon isn't pure white to avoid looking washed out
        vec3 horizonColor = mix(vec3(0.8, 0.9, 1.0), v_Color.rgb, 0.3);
        vec3 zenithColor = v_Color.rgb;
        
        // Smooth blend from horizon (h close to 0) to zenith
        float blend = clamp(pow(max(h, 0.0), 0.6) * 1.5, 0.0, 1.0);
        finalColor.rgb = mix(horizonColor, zenithColor, blend);
        finalColor.a = 1.0;
        
        // Apply fog to the sky near the horizon to blend with the world
        if (u_UseFog) {
             finalColor.rgb = mix(u_FogColor.rgb, finalColor.rgb, clamp(h * 4.0, 0.0, 1.0));
        }
    } else if (u_IsSky == 2) {
        // Celestial body (Sun/Moon). Their textures typically have a black background instead of an alpha channel.
        // We generate the alpha based on the maximum color intensity to make the black transparent!
        texColor.a = max(texColor.r, max(texColor.g, texColor.b));
        finalColor = texColor * v_Color;
    } else {
        if (u_AlphaTest && finalColor.a < 0.1) {
            discard;
        }

        if (u_UseFog && u_FogEnd > u_FogStart) {
            float fogFactor = clamp((u_FogEnd - v_FogDepth) / (u_FogEnd - u_FogStart), 0.0, 1.0);
            finalColor.rgb = mix(u_FogColor.rgb, finalColor.rgb, fogFactor);
        }
    }

    fragColor = finalColor;
}
)";

bool TerrainShader::init()
{
    if (inited) return true;
    inited = instance.loadFromSource(defaultTerrainVert, defaultTerrainFrag);
    if (!inited) {
        LOGE("Failed to load embedded Terrain shaders!\n");
    } else {
        LOGI("TerrainShader initialized successfully.\n");
    }
    return inited;
}

void TerrainShader::setupMVP(const Matrix4f& mvp)
{
    if (!inited) init();
    instance.bind();
    instance.setUniformMatrix4f("u_MVP", mvp);
    instance.setUniform1i("u_Texture", 0);
}

void TerrainShader::bind()
{
    if (!inited) init();
    instance.bind();
}

void TerrainShader::unbind()
{
    instance.unbind();
}
