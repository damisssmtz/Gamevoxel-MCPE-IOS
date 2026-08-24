#include "EntityShader.h"
#include "../../platform/log.h"

Shader EntityShader::instance;
bool EntityShader::inited = false;

// Embedded versions (for fallback or default)
static const char* defaultEntityVert = R"(#version 300 es
precision highp float;

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;
layout(location = 2) in vec4 a_Color;
layout(location = 3) in vec3 a_Normal;

uniform mat4 u_MVP;
uniform mat4 u_ModelView;
uniform vec4 u_LightColor;

out vec2 v_TexCoord;
out vec4 v_Color;
out float v_FogDepth;

void main() {
    v_TexCoord = a_TexCoord;
    v_Color = a_Color * u_LightColor;
    
    vec4 viewPos = u_ModelView * vec4(a_Position, 1.0);
    v_FogDepth = length(viewPos.xyz);
    
    gl_Position = u_MVP * vec4(a_Position, 1.0);
}
)";

static const char* defaultEntityFrag = R"(#version 300 es
precision highp float;

in vec2 v_TexCoord;
in vec4 v_Color;
in float v_FogDepth;

uniform sampler2D u_Texture;
uniform bool u_UseFog;
uniform vec4 u_FogColor;
uniform float u_FogStart;
uniform float u_FogEnd;
uniform vec4 u_OverlayColor;

out vec4 fragColor;

void main() {
    vec4 texColor = texture(u_Texture, v_TexCoord);
    if (texColor.a < 0.1) {
        discard;
    }

    vec4 finalColor = texColor * v_Color;
    finalColor.rgb = mix(finalColor.rgb, u_OverlayColor.rgb, u_OverlayColor.a);

    if (u_UseFog && u_FogEnd > u_FogStart) {
        float fogFactor = clamp((u_FogEnd - v_FogDepth) / (u_FogEnd - u_FogStart), 0.0, 1.0);
        finalColor.rgb = mix(u_FogColor.rgb, finalColor.rgb, fogFactor);
    }

    fragColor = finalColor;
}
)";

bool EntityShader::init()
{
    if (inited) return true;
    inited = instance.loadFromSource(defaultEntityVert, defaultEntityFrag);
    if (!inited) {
        LOGE("Failed to load embedded Entity shaders!\n");
    } else {
        LOGI("EntityShader initialized successfully.\n");
    }
    return inited;
}

void EntityShader::setupMVP(const Matrix4f& mvp, const Matrix4f& modelView)
{
    if (!inited) init();
    instance.bind();
    instance.setUniformMatrix4f("u_MVP", mvp);
    instance.setUniformMatrix4f("u_ModelView", modelView);
    instance.setUniform1i("u_Texture", 0);
}

void EntityShader::setLightColor(float r, float g, float b, float a)
{
    if (!inited) init();
    instance.setUniform4f("u_LightColor", r, g, b, a);
}

void EntityShader::setOverlayColor(float r, float g, float b, float a)
{
    if (!inited) init();
    instance.setUniform4f("u_OverlayColor", r, g, b, a);
}

void EntityShader::setFogParams(bool useFog, float start, float end, float r, float g, float b, float a)
{
    if (!inited) init();
    instance.setUniform1i("u_UseFog", useFog ? 1 : 0);
    instance.setUniform1f("u_FogStart", start);
    instance.setUniform1f("u_FogEnd", end);
    instance.setUniform4f("u_FogColor", r, g, b, a);
}

void EntityShader::bind()
{
    if (!inited) init();
    instance.bind();
}

void EntityShader::unbind()
{
    instance.unbind();
}
