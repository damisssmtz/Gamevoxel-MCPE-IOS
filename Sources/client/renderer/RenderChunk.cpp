#include "RenderChunk.h"

int RenderChunk::runningId = 0;

RenderChunk::RenderChunk() :
	vaoId(0),
	vboId(-1),
	vertexCount(0)
{
	id = ++runningId;
}

RenderChunk::RenderChunk( GLuint vboId_, int vertexCount_ )
:	vaoId(0),
	vboId(vboId_),
	vertexCount(vertexCount_)
{
	id = ++runningId;
}
