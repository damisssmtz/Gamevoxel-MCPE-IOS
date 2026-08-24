#ifndef NET_MINECRAFT_CLIENT_RENDERER__gles_H__
#define NET_MINECRAFT_CLIENT_RENDERER__gles_H__

#include "../../platform/log.h"
#include "../Options.h"

// Android should always run OPENGL_ES
#ifndef OPENGL_ES
#if defined(ANDROID) || defined(__APPLE__) || defined(RPI)
    #define OPENGL_ES
#endif
#endif

// Other systems might run it, if they #define OPENGL_ES
// #if defined(OPENGL_ES) // || defined(ANDROID)
	#define USE_VBO
	// DEPRECATED/REMOVED: The legacy display list fallback (#ifndef USE_VBO paths with glCallLists) 
	// is now considered dead code. Modern paths strictly use VBOs/VAOs.
	#define GL_QUADS 0x0007
    #if defined(__APPLE__)
        #import <OpenGLES/ES3/gl.h>
        #import <OpenGLES/ES3/glext.h>
    #elif defined(ANDROID) || defined(__EMSCRIPTEN__)
        #include <GLES/gl.h>
        #include <GLES/glext.h>
    #else
        #if defined(_WIN32)
            #include <winsock2.h>
            #include <ws2tcpip.h>
            #include <windows.h>
            #include <shellapi.h>
        #endif
        #include <glad/glad.h>

		// https://github.com/programmer1o1/MinecraftPE-v0.6.1/blob/main/handheld/src/client/renderer/gles.h#L135-L138
		#define glFogx(a,b)             glFogi(a,b)
		#define glOrthof(a,b,c,d,e,f)   glOrtho(a,b,c,d,e,f)
		#define glClearDepthf(x)        glClearDepth(x)
		#define glDepthRangef(a,b)      glDepthRange(a,b)
    #endif

    // Fallbacks for removed OpenGL ES 1.1 constants
    #ifndef GL_MODELVIEW
    #define GL_MODELVIEW 0x1700
    #endif
    #ifndef GL_PROJECTION
    #define GL_PROJECTION 0x1701
    #endif
    #ifndef GL_ALPHA_TEST
    #define GL_ALPHA_TEST 0x0BC0
    #endif
    #ifndef GL_PERSPECTIVE_CORRECTION_HINT
    #define GL_PERSPECTIVE_CORRECTION_HINT 0x0C50
    #endif
    #ifndef GL_FOG
    #define GL_FOG 0x0B60
    #endif
    #ifndef GL_SMOOTH
    #define GL_SMOOTH 0x1D01
    #endif
    #ifndef GL_FLAT
    #define GL_FLAT 0x1D00
    #endif

    // Extra missing constants
    #define GL_LIGHTING 0x0B50
    #define GL_LIGHT0 0x4000
    #define GL_LIGHT1 0x4001
    #define GL_COLOR_MATERIAL 0x0B57
    #define GL_RESCALE_NORMAL 0x803A
    #define GL_NORMALIZE 0x0BA1
    #define GL_AMBIENT 0x1200
    #define GL_DIFFUSE 0x1201
    #define GL_SPECULAR 0x1202
    #define GL_POSITION 0x1203
    #define GL_LIGHT_MODEL_AMBIENT 0x0B53

    #define GL_VERTEX_ARRAY 0x8074
    #define GL_NORMAL_ARRAY 0x8075
    #define GL_COLOR_ARRAY 0x8076
    #define GL_TEXTURE_COORD_ARRAY 0x8078

    #define GL_FOG_MODE 0x0B65
    #define GL_EXP 0x0800

    #define GL_PROJECTION_MATRIX 0x0BA7
    #define GL_MODELVIEW_MATRIX 0x0BA6
// #else
//     // Uglyness to fix redeclaration issues
//     #ifdef WIN32
// 	   #include <winsock2.h>
// 	   #include <Windows.h>
// 	#endif
// 	#include <gl/glew.h>
// 	#include <gl/GL.h>

// 	#define glFogx(a,b)	glFogi(a,b)
// 	#define glOrthof(a,b,c,d,e,f) glOrtho(a,b,c,d,e,f)
// #endif


#define GLERRDEBUG 1
#if GLERRDEBUG
//#define GLERR(x) if((x) != 0) { LOGI("GLError: " #x "(%d)\n", __LINE__) }
#define GLERR(x) do { const int errCode = glGetError(); if (errCode != 0) LOGE("OpenGL ERROR @%d: #%d @ (%s : %d)\n", x, errCode, __FILE__, __LINE__); } while (0)
#else
#define GLERR(x) x
#endif

void anGenBuffers(GLsizei n, GLuint* buffer);

#ifdef USE_VBO
#define drawArrayVT_NoState drawArrayVT
#define drawArrayVTC_NoState drawArrayVTC
#define drawArrayVTN_NoState drawArrayVTN          // ← AÑADIR

void drawArrayVT(int bufferId, int vertices, int vertexSize = 36, unsigned int mode = GL_TRIANGLES);
#ifndef drawArrayVT_NoState
//void drawArrayVT_NoState(int bufferId, int vertices, int vertexSize = 36);
#endif

void drawArrayVTC(int bufferId, int vertices, int vertexSize = 36);
#ifndef drawArrayVTC_NoState
void drawArrayVTC_NoState(int bufferId, int vertices, int vertexSize = 36);
#endif

void drawArrayVTN(int bufferId, int vertices, int vertexSize = 36);   // ← AÑADIR
#ifndef drawArrayVTN_NoState
void drawArrayVTN_NoState(int bufferId, int vertices, int vertexSize = 36);  // ← AÑADIR
#endif
#endif

void glInit();
void gluPerspective(GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar);
int glhUnProjectf(	float winx, float winy, float winz,
					float *modelview, float *projection,
					int *viewport, float *objectCoordinate);

// Used for "debugging" (...). Obviously stupid dependency on Options (and ugly gl*2 calls).
#ifdef GLDEBUG
	#define glTranslatef2(x, y, z) do{ if (Options::debugGl) LOGI("glTrans @ %s:%d: %f,%f,%f\n", __FILE__, __LINE__, x, y, z); glTranslatef(x, y, z); GLERR(0); } while(0)
	#define glRotatef2(a, x, y, z) do{ if (Options::debugGl) LOGI("glRotat @ %s:%d: %f,%f,%f,%f\n", __FILE__, __LINE__, a, x, y, z); glRotatef(a, x, y, z); GLERR(1); } while(0)
	#define glScalef2(x, y, z) do{ if (Options::debugGl) LOGI("glScale @ %s:%d: %f,%f,%f\n", __FILE__, __LINE__, x, y, z); glScalef(x, y, z); GLERR(2); } while(0)
	#define glPushMatrix2() do{ if (Options::debugGl) LOGI("glPushM @ %s:%d\n", __FILE__, __LINE__); glPushMatrix(); GLERR(3); } while(0)
	#define glPopMatrix2() do{ if (Options::debugGl) LOGI("glPopM  @ %s:%d\n", __FILE__, __LINE__); glPopMatrix(); GLERR(4); } while(0)
	#define glLoadIdentity2() do{ if (Options::debugGl) LOGI("glLoadI @ %s:%d\n", __FILE__, __LINE__); glLoadIdentity(); GLERR(5); } while(0)

	#define glVertexPointer2(a, b, c, d) do{ if (Options::debugGl) LOGI("glVertexPtr @ %s:%d : %d\n", __FILE__, __LINE__, 0); glVertexPointer(a, b, c, d); GLERR(6); } while(0)
	#define glColorPointer2(a, b, c, d) do{ if (Options::debugGl) LOGI("glColorPtr @ %s:%d : %d\n", __FILE__, __LINE__, 0); glColorPointer(a, b, c, d); GLERR(7); } while(0)
	#define glTexCoordPointer2(a, b, c, d) do{ if (Options::debugGl) LOGI("glTexPtr @ %s:%d : %d\n", __FILE__, __LINE__, 0); glTexCoordPointer(a, b, c, d); GLERR(8); } while(0)
	#define glEnableClientState2(s) do{ if (Options::debugGl) LOGI("glEnableClient @ %s:%d : %d\n", __FILE__, __LINE__, 0); glEnableClientState(s); GLERR(9); } while(0)
	#define glDisableClientState2(s) do{ if (Options::debugGl) LOGI("glDisableClient @ %s:%d : %d\n", __FILE__, __LINE__, 0); glDisableClientState(s); GLERR(10); } while(0)
	#define glDrawArrays2(m, o, v) do{ if (Options::debugGl) LOGI("glDrawA @ %s:%d : %d\n", __FILE__, __LINE__, 0); glDrawArrays(m,o,v); GLERR(11); } while(0)

	#define glTexParameteri2(m, o, v) do{ if (Options::debugGl) LOGI("glTexParameteri @ %s:%d : %d\n", __FILE__, __LINE__, v); glTexParameteri(m,o,v); GLERR(12); } while(0)
	#define glTexImage2D2(a,b,c,d,e,f,g,height,i) do{ if (Options::debugGl) LOGI("glTexImage2D @ %s:%d : %d\n", __FILE__, __LINE__, 0); glTexImage2D(a,b,c,d,e,f,g,height,i); GLERR(13); } while(0)
	#define glTexSubImage2D2(a,b,c,d,e,f,g,height,i) do{ if (Options::debugGl) LOGI("glTexSubImage2D @ %s:%d : %d\n", __FILE__, __LINE__, 0); glTexSubImage2D(a,b,c,d,e,f,g,height,i); GLERR(14); } while(0)
	#define glGenBuffers2(s, id) do{ if (Options::debugGl) LOGI("glGenBuffers @ %s:%d : %d\n", __FILE__, __LINE__, id); anGenBuffers(s, id); GLERR(15); } while(0)
	#define glBindBuffer2(s, id) do{ if (Options::debugGl) LOGI("glBindBuffer @ %s:%d : %d\n", __FILE__, __LINE__, id); glBindBuffer(s, id); GLERR(16); } while(0)
	#define glBufferData2(a, b, c, d) do{ if (Options::debugGl) LOGI("glBufferData @ %s:%d : %d\n", __FILE__, __LINE__, d); glBufferData(a, b, c, d); GLERR(17); } while(0)
	#define glBindTexture2(m, z) do{ if (Options::debugGl) LOGI("glBindTexture @ %s:%d : %d\n", __FILE__, __LINE__, z); glBindTexture(m, z); GLERR(18); } while(0)

	#define glEnable2(s) do{ if (Options::debugGl) LOGI("glEnable @ %s:%d : %d\n", __FILE__, __LINE__, s); glEnable(s); GLERR(19); } while(0)
	#define glDisable2(s) do{ if (Options::debugGl) LOGI("glDisable @ %s:%d : %d\n", __FILE__, __LINE__, s); glDisable(s); GLERR(20); } while(0)
	
	#define glColor4f2(r, g, b, a) do{ if (Options::debugGl) LOGI("glColor4f2 @ %s:%d : (%f,%f,%f,%f)\n", __FILE__, __LINE__, r,g,b,a); glColor4f(r,g,b,a); GLERR(21); } while(0)

	//#define glBlendMode2(s) do{ if (Options::debugGl) LOGI("glEnable @ %s:%d : %d\n", __FILE__, __LINE__, s); glEnable(s); GLERR(19); } while(0)
	#define glBlendFunc2(src, dst) do{ if (Options::debugGl) LOGI("glBlendFunc @ %s:%d : %d - %d\n", __FILE__, __LINE__, src, dst); glBlendFunc(src, dst); GLERR(23); } while(0)
	#define glShadeModel2(s) do{ if (Options::debugGl) LOGI("glShadeModel @ %s:%d : %d\n", __FILE__, __LINE__, s); glShadeModel(s); GLERR(25); } while(0)
#else
	#include "GLState.h"

	#define glMatrixMode(m)	GLState::matrixMode(m == GL_PROJECTION ? GLState::PROJECTION : GLState::MODELVIEW)
	#define glTranslatef2(x,y,z) GLState::translate(x,y,z)
	#define glTranslatef(x,y,z) GLState::translate(x,y,z)
	#define glRotatef2(a,x,y,z)	GLState::rotate(a,x,y,z)
	#define glRotatef(a,x,y,z)	GLState::rotate(a,x,y,z)
	#define glScalef2(x,y,z)	GLState::scale(x,y,z)
	#define glScalef(x,y,z)	    GLState::scale(x,y,z)
	#define glPushMatrix2()	    GLState::pushMatrix()
	#define glPushMatrix()	    GLState::pushMatrix()
	#define glPopMatrix2()	    GLState::popMatrix()
	#define glPopMatrix()	    GLState::popMatrix()
	#define glLoadIdentity2()   GLState::loadIdentity()
	#define glLoadIdentity()    GLState::loadIdentity()
    #define glColor4f(r,g,b,a)  GLState::color(r,g,b,a)
    #define glColor4f2(r,g,b,a) GLState::color(r,g,b,a)
    #define glAlphaFunc(func, ref)
    #define glShadeModel(mode)
    #define glHint(target, mode)
    #define glFogf(pname, param)
    #define glFogfv(pname, params)
    #define glFogi(pname, param)

	#define glVertexPointer2(...)
	#define glColorPointer2(...)
	#define glTexCoordPointer2(...)
	#define glEnableClientState2(...)
	#define glDisableClientState2(...)
	#define glDrawArrays2(mode, first, count) do { GLState::bindFallbackShader(); glDrawArrays(mode, first, count); } while(0)
    
    // Extra missing function macros
    #define glVertexPointer(...)
    #define glTexCoordPointer(...)
    #define glColorPointer(...)
    #define glNormalPointer(...)
    #define glNormal3f(...)
    #define glMultMatrixf(m) GLState::multMatrix(m)
    #define glOrthof(l,r,b,t,n,f) GLState::ortho(l,r,b,t,n,f)

    #ifndef GL_TEXTURE_2D
    #define GL_TEXTURE_2D 0x0DE1
    #endif

    #define glFogx(...)
    #define glLightfv(...)
    #define glLightModelfv(...)


	#define glTexParameteri2 glTexParameteri
	#define glTexImage2D2	 glTexImage2D
	#define glTexSubImage2D2 glTexSubImage2D
	#define glGenBuffers2	 anGenBuffers
	#define glBindBuffer2	 glBindBuffer
	#define glBufferData2	 glBufferData
	#define glBindTexture2	 glBindTexture

	#define glEnable2		glEnable
	#define glDisable2		glDisable

	#define glColor4f2		glColor4f
	#define glBlendFunc2	glBlendFunc
	#define glShadeModel2	glShadeModel
#endif

//
// Extensions
//
#ifdef WIN32
	#define glGetProcAddress(a) wglGetProcAddress(a)
#else
	#define glGetProcAddress(a) (void*(0))
#endif



#endif /*NET_MINECRAFT_CLIENT_RENDERER__gles_H__ */