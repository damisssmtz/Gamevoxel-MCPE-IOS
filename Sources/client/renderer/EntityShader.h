#ifndef NET_MINECRAFT_CLIENT_RENDERER__EntityShader_H__
#define NET_MINECRAFT_CLIENT_RENDERER__EntityShader_H__

#include "Shader.h"
#include "../../util/Matrix4f.h"

class EntityShader
{
public:
    static bool init();
    static void setupMVP(const Matrix4f& mvp, const Matrix4f& modelView);
    static void setLightColor(float r, float g, float b, float a);
    static void setOverlayColor(float r, float g, float b, float a);
    static void setFogParams(bool useFog, float start, float end, float r, float g, float b, float a);
    static void bind();
    static void unbind();

private:
    static Shader instance;
    static bool inited;
};

#endif /* NET_MINECRAFT_CLIENT_RENDERER__EntityShader_H__ */
