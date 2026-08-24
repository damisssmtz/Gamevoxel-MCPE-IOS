#version 300 es
precision highp float;

in vec2 v_TexCoord;
in vec4 v_Color;
in float v_FogDepth;
in vec3 v_WorldPos;
in vec3 v_ViewPos;
in vec3 v_WaveNormalView;
in vec3 v_WorldUpView;
in vec3 v_WorldRightView;
in vec3 v_WorldForwardView;
in float v_WaveHeight;
in float v_TopFace;

uniform bool u_UseFog;
uniform vec4 u_FogColor;
uniform float u_FogStart;
uniform float u_FogEnd;
uniform float u_Time;

out vec4 fragColor;

const vec3 SHALLOW =
vec3(0.08, 0.58, 0.82);

const vec3 DEEP =
vec3(0.015, 0.12, 0.32);

const vec3 FOAM =
vec3(0.95, 0.98, 1.0);

const float F0 = 0.02;

float hash(vec2 p)
{
    p = fract(p * vec2(234.34, 876.21));
    p += dot(p, p + 34.5);
    return fract(p.x * p.y);
}

vec3 waveField(vec2 p, float time)
{
    const float amplitude = 0.038;
    const float frequency = 1.35;
    vec2 d1 = normalize(vec2(1.0, 0.18));
    vec2 d2 = normalize(vec2(-0.28, 1.0));
    vec2 d3 = normalize(vec2(0.72, -0.69));
    float a1 = dot(p, d1) * frequency + time * 0.75;
    float a2 = dot(p, d2) * frequency * 1.37 - time * 0.53;
    float a3 = dot(p, d3) * frequency * 1.91 + time * 0.31;
    float height = (sin(a1) + sin(a2) * 0.52 + sin(a3) * 0.22) * amplitude;
    vec2 gradient =
        (d1 * cos(a1) +
         d2 * cos(a2) * 0.52 * 1.37 +
         d3 * cos(a3) * 0.22 * 1.91) * amplitude;
    return vec3(height, gradient);
}

void main()
{
    vec3 field = waveField(v_WorldPos.xz, u_Time);
    float wave = field.x;
    vec3 N = normalize(
        v_WorldUpView -
        v_WorldRightView * field.y -
        v_WorldForwardView * field.z
    );
    vec3 V = normalize(-v_ViewPos);

    float NdotV = max(dot(N, V), 0.0);

    float fresnel =
        F0 +
        (1.0 - F0) *
        pow(1.0 - NdotV, 5.0);

    float dist = length(v_ViewPos);

    float depth =
        clamp(
            dist * 0.015 +
            (1.0 - NdotV) * 0.35,
            0.0,
            1.0
        );

    vec3 waterColor =
        mix(SHALLOW, DEEP, depth);

    if(v_Color.a > 0.01)
    {
        waterColor *= v_Color.rgb;
    }

    float micro =
        sin(v_WorldPos.x * 7.0 + u_Time * 2.0) *
        sin(v_WorldPos.z * 5.0 - u_Time * 1.5);

    micro = micro * 0.5 + 0.5;

    waterColor += micro * 0.03;

    float crest =
        smoothstep(
            0.015,
            0.04,
            wave
        );

    float foam = crest * v_TopFace;

    foam *= v_TopFace;

    vec3 reflection =
        vec3(0.9, 0.97, 1.0);

    vec3 color =
        mix(
            waterColor,
            reflection,
            fresnel * 0.45
        );

    color =
        mix(
            color,
            FOAM,
            foam * 0.5
        );

    float sparkle =
        pow(max(dot(reflect(-V, N), vec3(0.3,0.8,0.4)),0.0),64.0);

    color += sparkle * 0.18;

    float alpha =
        0.55 +
        fresnel * 0.25 +
        foam * 0.15;

    alpha = clamp(alpha, 0.55, 0.92);

    if(u_UseFog && u_FogEnd > u_FogStart)
    {
        float fog =
            clamp(
                (u_FogEnd - v_FogDepth) /
                (u_FogEnd - u_FogStart),
                0.0,
                1.0
            );

        color =
            mix(
                u_FogColor.rgb,
                color,
                fog
            );
    }

    fragColor = vec4(color, alpha);
}