#include "render_text.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fontstash.h>

#include "../vmlib/mat44.hpp"

struct GLFONScontext {
	GLuint tex;
	GLuint vao, vboVerts, vboTexCoords, vboColors, shaderProgram;
	int width, height;
};
typedef struct GLFONScontext GLFONScontext;

int glfons__renderCreate(void* userPtr, int width, int height)
{
	GLFONScontext* gl = (GLFONScontext*)userPtr;
	// Create may be called multiple times, delete existing texture.
	if (gl->tex != 0) {
		glDeleteTextures(1, &gl->tex);
		gl->tex = 0;
	}
	gl->width = width;
	gl->height = height;

	glGenTextures(1, &gl->tex);
	glBindTexture(GL_TEXTURE_2D, gl->tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	return gl->tex;
}

static int glfons__renderResize(void* userPtr, int width, int height)
{
	// Reuse create to resize too.
	return glfons__renderCreate(userPtr, width, height);
}

static void glfons__renderUpdate(void* userPtr, int* rect, const unsigned char* data)
{
	GLFONScontext* gl = (GLFONScontext*)userPtr;
	int w = rect[2] - rect[0];
	int h = rect[3] - rect[1];

	if (gl->tex == 0) return;
	glBindTexture(GL_TEXTURE_2D, gl->tex);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, gl->width);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, rect[0]);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, rect[1]);

	glTexSubImage2D(GL_TEXTURE_2D, 0, rect[0], rect[1], w, h, GL_RED, GL_UNSIGNED_BYTE, data);


	// Reset to default OpenGL states
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);      // Default is 4
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);    // Default is 0
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);   // Default is 0
	glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);     // Default is 0
}

static void glfons__renderDraw(void* userPtr, const float* verts, const float* tcoords, const unsigned int* colors, int nverts) {
	GLFONScontext* gl = (GLFONScontext*)userPtr;

	if (gl->tex == 0) return;

	// Bind texture
	glBindTexture(GL_TEXTURE_2D, gl->tex);

	// Bind VAO
	glBindVertexArray(gl->vao);

	// Upload vertex positions
	glBindBuffer(GL_ARRAY_BUFFER, gl->vboVerts);
	glBufferData(GL_ARRAY_BUFFER, nverts * 2 * sizeof(float), verts, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glEnableVertexAttribArray(0);

	// Upload texture coordinates
	glBindBuffer(GL_ARRAY_BUFFER, gl->vboTexCoords);
	glBufferData(GL_ARRAY_BUFFER, nverts * 2 * sizeof(float), tcoords, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glEnableVertexAttribArray(1);

	// Upload colours
	glBindBuffer(GL_ARRAY_BUFFER, gl->vboColors);
	glBufferData(GL_ARRAY_BUFFER, nverts * sizeof(unsigned int), colors, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (void*)0); // GL_TRUE normalises colours
	glEnableVertexAttribArray(2);

	// Use the shader program
	glUseProgram(gl->shaderProgram);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Compute and load ortho projection matrix
	float projMatrix[16] = {
	2.0f / gl->width, 0.0f, 0.0f, 0.0f,
	0.0f, -2.0f / gl->height, 0.0f, 0.0f,
	0.0f, 0.0f, -1.0f, 0.0f,
	-1.0f, 1.0f, 0.0f, 1.0f
	};

	glUniformMatrix4fv(0, 1, GL_FALSE, projMatrix);

	// Set the texture uniform
	glUniform1i(glGetUniformLocation(gl->shaderProgram, "uTexture"), 0);

	// Draw triangles
	glDrawArrays(GL_TRIANGLES, 0, nverts);

	// Restore OpenGL state
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);

	// Unbind everything (optional, for cleanliness)
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}


static void glfons__renderDelete(void* userPtr)
{
	GLFONScontext* gl = (GLFONScontext*)userPtr;

	// Delete texture if it exists
	if (gl->tex != 0) {
		glDeleteTextures(1, &gl->tex);
		gl->tex = 0;
	}

	// Delete VAO if it exists
	if (gl->vao != 0) {
		glDeleteVertexArrays(1, &gl->vao);
		gl->vao = 0;
	}

	// Delete VBOs if they exist
	if (gl->vboVerts != 0) {
		glDeleteBuffers(1, &gl->vboVerts);
		gl->vboVerts = 0;
	}
	if (gl->vboTexCoords != 0) {
		glDeleteBuffers(1, &gl->vboTexCoords);
		gl->vboTexCoords = 0;
	}
	if (gl->vboColors != 0) {
		glDeleteBuffers(1, &gl->vboColors);
		gl->vboColors = 0;
	}

	// Free the GLFONScontext structure
	free(gl);
}



FONScontext* glfonsCreate(int width, int height, int flags, GLuint shaderProgramID)
{
	FONSparams params;
	GLFONScontext* gl;



	gl = (GLFONScontext*)malloc(sizeof(GLFONScontext));
	if (gl == NULL) goto error;
	memset(gl, 0, sizeof(GLFONScontext));

	GLint linked;
	glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &linked);
	if (!linked) {
		fprintf(stderr, "Shader program failed to link.\n");
		goto error;
	}

	// Generate VAO and VBOs
	glGenVertexArrays(1, &gl->vao);
	glGenBuffers(1, &gl->vboVerts);
	glGenBuffers(1, &gl->vboTexCoords);
	glGenBuffers(1, &gl->vboColors);

	// Create and compile shaders
	gl->shaderProgram = shaderProgramID;

	memset(&params, 0, sizeof(params));
	params.width = width;
	params.height = height;
	params.flags = (unsigned char)flags;
	params.renderCreate = glfons__renderCreate;
	params.renderResize = glfons__renderResize;
	params.renderUpdate = glfons__renderUpdate;
	params.renderDraw = glfons__renderDraw;
	params.renderDelete = glfons__renderDelete;
	params.userPtr = gl;

	return fonsCreateInternal(&params);

error:
	if (gl->vao) glDeleteVertexArrays(1, &gl->vao);
	if (gl->vboVerts) glDeleteBuffers(1, &gl->vboVerts);
	if (gl->vboTexCoords) glDeleteBuffers(1, &gl->vboTexCoords);
	if (gl->vboColors) glDeleteBuffers(1, &gl->vboColors);
	if (gl) free(gl);
	return NULL;

}

void glfonsDelete(FONScontext* ctx)
{
	fonsDeleteInternal(ctx);
}

unsigned int glfonsRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	return (r) | (g << 8) | (b << 16) | (a << 24);
}


void renderText(FONScontext* fsContext, const char* text, float x, float y, float fontSize, unsigned int color, int font)
{
	// Set the font size
	fonsSetSize(fsContext, fontSize);

	// Set the colour (RGBA format)
	fonsSetColor(fsContext, color);

	fonsSetFont(fsContext, font);

	// Draw the text at position (x, y)
	fonsDrawText(fsContext, x, y, text, NULL);
}