#include "RenderList.h"

#include "gles.h"
#include "RenderChunk.h"
#include "Tesselator.h"


RenderList::RenderList()
	:	inited(false),
	rendered(false)
{
	lists = new int[MAX_NUM_OBJECTS];
	rlists = new RenderChunk[MAX_NUM_OBJECTS];

	for (int i = 0; i < MAX_NUM_OBJECTS; ++i)
		rlists[i].vboId = -1;
}

RenderList::~RenderList() {
	delete[] lists;
	delete[] rlists;
}

void RenderList::init(float xOff, float yOff, float zOff) {
	inited = true;
	listIndex = 0;

	this->xOff = (float) xOff;
	this->yOff = (float) yOff;
	this->zOff = (float) zOff;
}

void RenderList::add(int list) {
	if (listIndex >= MAX_NUM_OBJECTS)
		return;

	lists[listIndex++] = list;
}

void RenderList::addR(const RenderChunk& chunk) {
	if (listIndex >= MAX_NUM_OBJECTS)
		return;

	rlists[listIndex++] = chunk;
}

void RenderList::render() {

	if (!inited) return;
	if (!rendered) {
		bufferLimit = listIndex;
		listIndex = 0;
		rendered = true;
	}
	if (listIndex < bufferLimit) {
		glPushMatrix2();
		glTranslatef2(-xOff, -yOff, -zOff);

		#ifndef USE_VBO
			glCallLists(bufferLimit, GL_UNSIGNED_INT, lists);
		#else
			renderChunks();
		#endif/*!USE_VBO*/

		glPopMatrix2();
	}
}

void RenderList::renderChunks() {
	glShadeModel2(GL_SMOOTH);
	const int Stride = VertexSizeBytes;

	for (int i = 0; i < bufferLimit; ++i) {
		RenderChunk& rc = rlists[i];

		if (rc.vaoId == 0) {
			// VAO is now generated inside Chunk::rebuild to avoid memory leaks
			continue;
		}

		glPushMatrix2();
		glTranslatef2(rc.pos.x, rc.pos.y, rc.pos.z);
		
		GLState::chunkOffset[0] = rc.pos.x;
		GLState::chunkOffset[1] = rc.pos.y;
		GLState::chunkOffset[2] = rc.pos.z;
		
		glBindVertexArray(rc.vaoId);
		glDrawArrays2(GL_TRIANGLES, 0, rc.vertexCount);
		glBindVertexArray(0);

		glPopMatrix2();
	}
}

void RenderList::clear() {
	inited = false;
	rendered = false;
}