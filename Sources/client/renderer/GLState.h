#ifndef NET_MINECRAFT_CLIENT_RENDERER__GLState_H__
#define NET_MINECRAFT_CLIENT_RENDERER__GLState_H__

#include "../../util/Matrix4f.h"
#include <vector>

class GLState {
public:
    static bool useWaterShader;
    static float chunkOffset[3];
    enum MatrixMode {
        MODELVIEW,
        PROJECTION
    };

    static void matrixMode(MatrixMode mode);
    static void loadIdentity();
    static void pushMatrix();
    static void popMatrix();
    static void translate(float x, float y, float z);
    static void rotate(float angleDegrees, float x, float y, float z);
    static void scale(float x, float y, float z);
    static void multMatrix(const float* m);
    static void ortho(float l, float r, float b, float t, float n, float f);
    static void getFloatv(unsigned int pname, float* params);
    static const float* getColor();
    static void bindFallbackShader();
    
    static void setTextureEnabled(bool enabled);
    static bool isTextureEnabled();

    static void setAlphaTestEnabled(bool enabled);
    static bool isAlphaTestEnabled();

    static void setSkyMode(int mode);
    static int getSkyMode();

    static Matrix4f getModelView();
    static Matrix4f getProjection();
    static Matrix4f getMVP();
    static void color(float r, float g, float b, float a);

private:
    static MatrixMode currentMode;
    static std::vector<Matrix4f> modelViewStack;
    static std::vector<Matrix4f> projectionStack;

    static std::vector<Matrix4f>& currentStack();
};

#endif /* NET_MINECRAFT_CLIENT_RENDERER__GLState_H__ */
