#include "gles.h"
#include "GLState.h"

GLState::MatrixMode GLState::currentMode = MODELVIEW;
std::vector<Matrix4f> GLState::modelViewStack = { Matrix4f::identity() };
std::vector<Matrix4f> GLState::projectionStack = { Matrix4f::identity() };

std::vector<Matrix4f>& GLState::currentStack() {
    return (currentMode == MODELVIEW) ? modelViewStack : projectionStack;
}

void GLState::matrixMode(MatrixMode mode) {
    currentMode = mode;
}

void GLState::loadIdentity() {
    currentStack().back().setIdentity();
}

void GLState::pushMatrix() {
    currentStack().push_back(currentStack().back());
}

void GLState::popMatrix() {
    if (currentStack().size() > 1) {
        currentStack().pop_back();
    }
}

void GLState::translate(float x, float y, float z) {
    Matrix4f& top = currentStack().back();
    top = top * Matrix4f::createTranslation(x, y, z);
}

void GLState::rotate(float angleDegrees, float x, float y, float z) {
    Matrix4f& top = currentStack().back();
    top = top * Matrix4f::createRotation(angleDegrees, x, y, z);
}

void GLState::scale(float x, float y, float z) {
    Matrix4f& top = currentStack().back();
    top = top * Matrix4f::createScale(x, y, z);
}

void GLState::multMatrix(const float* m) {
    Matrix4f mat;
    std::memcpy(mat.m, m, 16 * sizeof(float));
    currentStack().back() = currentStack().back() * mat;
}

void GLState::ortho(float l, float r, float b, float t, float n, float f) {
    currentStack().back() = currentStack().back() * Matrix4f::createOrtho(l, r, b, t, n, f);
}

void GLState::getFloatv(unsigned int pname, float* params) {
    if (pname == 0x0BA7) { // GL_PROJECTION_MATRIX
        const float* p = getProjection().getValues();
        for (int i = 0; i < 16; i++) params[i] = p[i];
    } else if (pname == 0x0BA6) { // GL_MODELVIEW_MATRIX
        const float* m = getModelView().getValues();
        for (int i = 0; i < 16; i++) params[i] = m[i];
    }
}

Matrix4f GLState::getModelView() {
    return modelViewStack.back();
}

Matrix4f GLState::getProjection() {
    return projectionStack.back();
}

Matrix4f GLState::getMVP() {
    return getProjection() * getModelView();
}
static float globalColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};

void GLState::color(float r, float g, float b, float a) {
    globalColor[0] = r;
    globalColor[1] = g;
    globalColor[2] = b;
    globalColor[3] = a;
}

const float* GLState::getColor() {
    return globalColor;
}

static bool globalTextureEnabled = true;

void GLState::setTextureEnabled(bool enabled) {
    globalTextureEnabled = enabled;
}

bool GLState::isTextureEnabled() {
    return globalTextureEnabled;
}

static bool globalAlphaTestEnabled = false;

void GLState::setAlphaTestEnabled(bool enabled) {
    globalAlphaTestEnabled = enabled;
}

bool GLState::isAlphaTestEnabled() {
    return globalAlphaTestEnabled;
}

static int globalSkyMode = 0;

void GLState::setSkyMode(int mode) {
    globalSkyMode = mode;
}

int GLState::getSkyMode() {
    return globalSkyMode;
}

#include "TerrainShader.h"
#include "WaterShader.h"

bool GLState::useWaterShader = false;
float GLState::chunkOffset[3] = {0, 0, 0};

void GLState::bindFallbackShader() {
    if (useWaterShader) {
        WaterShader::setupMVP(getMVP());
        WaterShader::instance.setUniformMatrix4f("u_ModelView", getModelView());
        WaterShader::instance.setUniform1i("u_UseFog", 0);
        WaterShader::instance.setUniform3f("u_ChunkPos", chunkOffset[0], chunkOffset[1], chunkOffset[2]);
        // Note: u_Time is already updated globally in GameRenderer
    } else {
        TerrainShader::setupMVP(getMVP());
        TerrainShader::instance.setUniformMatrix4f("u_ModelView", getModelView());
        TerrainShader::instance.setUniform1i("u_UseFog", 0);
        TerrainShader::instance.setUniform1i("u_UseTexture", globalTextureEnabled ? 1 : 0);
        TerrainShader::instance.setUniform1i("u_AlphaTest", globalAlphaTestEnabled ? 1 : 0);
        TerrainShader::instance.setUniform1i("u_IsSky", globalSkyMode);
    }
    
    // Set default vertex color (used if the color array is disabled)
    glVertexAttrib4f(2, globalColor[0], globalColor[1], globalColor[2], globalColor[3]);
}
