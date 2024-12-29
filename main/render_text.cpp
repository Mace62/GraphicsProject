#include "render_text.hpp"
//
//#include <fontstash.h>
//#include <GLFW/glfw3.h>
//#include <string>
//#include <filesystem>
//
//
//#include <iostream>
//
//#include "../support/program.hpp"
//
//// Fontstash context and OpenGL resources
//FONScontext* fs;
//static GLuint fontTextureId = 0; // Texture ID for the font bitmap
//static GLuint vao = 0, vbo = 0;  // Global VAO and VBO
//static GLuint textShaderID = 0;
//
//
//static std::vector<float> interleaveVertexData(const float* verts, const float* tcoords, const unsigned int* colors, int nverts) {
//    std::vector<float> interleavedData(nverts * 5);  // 2 pos + 2 tex + 1 packed color = 5 floats per vertex
//    for (int i = 0; i < nverts; i++) {
//        interleavedData[i * 5 + 0] = verts[i * 2 + 0];     // Position x
//        interleavedData[i * 5 + 1] = verts[i * 2 + 1];     // Position y
//        interleavedData[i * 5 + 2] = tcoords[i * 2 + 0];   // Texture u
//        interleavedData[i * 5 + 3] = tcoords[i * 2 + 1];   // Texture v
//        // Pack color into a float
//        unsigned int color = colors[i];
//        float* colorAsFloat = reinterpret_cast<float*>(&color);
//        interleavedData[i * 5 + 4] = *colorAsFloat;
//    }
//    return interleavedData;
//}
//
//
//static int glfons__renderCreate(void* userPtr, int width, int height) {
//    std::cout << "Creating font texture: " << width << "x" << height << std::endl;
//
//    glGenTextures(1, &fontTextureId);
//    glBindTexture(GL_TEXTURE_2D, fontTextureId);
//
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
//
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//
//    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
//
//    // Verify texture creation
//    if (glIsTexture(fontTextureId)) {
//        std::cout << "Font texture created successfully. ID: " << fontTextureId << std::endl;
//    }
//    else {
//        std::cerr << "Failed to create font texture!" << std::endl;
//    }
//
//    glGenVertexArrays(1, &vao);
//    glGenBuffers(1, &vbo);
//
//    glBindVertexArray(vao);
//    glBindBuffer(GL_ARRAY_BUFFER, vbo);
//
//    // Position attribute (2 floats)
//    glEnableVertexAttribArray(0);
//    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
//
//    // Texture coordinate attribute (2 floats)
//    glEnableVertexAttribArray(1);
//    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(2 * sizeof(float)));
//
//    // Color attribute (4 separate components)
//    glEnableVertexAttribArray(2);
//    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(4 * sizeof(float)));
//
//    // Make sure to keep texture in RED format for font atlas
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
//
//    // Check for errors
//    GLenum error = glGetError();
//    if (error != GL_NO_ERROR) {
//        std::cerr << "OpenGL error in renderCreate: 0x" << std::hex << error << std::endl;
//    }
//
//    return fontTextureId;
//}
//
//static int glfons__renderResize(void* userPtr, int width, int height) {
//    glBindTexture(GL_TEXTURE_2D, fontTextureId);
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
//    return 1;
//}
//
//static void glfons__renderUpdate(void* userPtr, int* rect, const unsigned char* data) {
//    std::cout << "Updating texture region:\n";
//    std::cout << "  x: " << rect[0] << ", y: " << rect[1] << "\n";
//    std::cout << "  width: " << rect[2] << ", height: " << rect[3] << "\n";
//
//    // Print first few bytes of texture data
//    std::cout << "First few bytes: ";
//    for (int i = 0; i < std::min(10, rect[2]); i++) {
//        std::cout << (int)data[i] << " ";
//    }
//    std::cout << "\n";
//
//    glBindTexture(GL_TEXTURE_2D, fontTextureId);
//    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
//    glTexSubImage2D(GL_TEXTURE_2D, 0, rect[0], rect[1], rect[2], rect[3], GL_RED, GL_UNSIGNED_BYTE, data);
//}
//
////static void glfons__renderDraw(void* userPtr, const float* verts, const float* tcoords, const unsigned int* colors, int nverts) {
////    // Ensure we have a valid shader
////    if (textShaderID == 0) {
////        std::cerr << "Error: Text shader not initialized!" << std::endl;
////        return;
////    }
////
////    glUseProgram(textShaderID);
////
////    // Bind texture and enable blending
////    glBindTexture(GL_TEXTURE_2D, fontTextureId);
////    glEnable(GL_BLEND);
////    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
////
////    // Interleave vertex data
////    std::vector<float> interleavedData = interleaveVertexData(verts, tcoords, colors, nverts);
////
////    // Update buffer data
////    glBindVertexArray(vao);
////    glBindBuffer(GL_ARRAY_BUFFER, vbo);
////    glBufferData(GL_ARRAY_BUFFER, interleavedData.size() * sizeof(float), interleavedData.data(), GL_STREAM_DRAW);
////
////    // Draw
////    glDrawArrays(GL_TRIANGLES, 0, nverts);
////
////    // Cleanup state
////    glBindVertexArray(0);
////    glUseProgram(0);
////    glDisable(GL_BLEND);
////}
//
//static void glfons__renderDraw(void* userPtr, const float* verts, const float* tcoords, const unsigned int* colors, int nverts) {
//    glUseProgram(textShaderID);
//    
//    GLint viewport[4];
//    glGetIntegerv(GL_VIEWPORT, viewport);
//    float viewportWidth = viewport[2];
//    float viewportHeight = viewport[3];
//    
//    glEnable(GL_BLEND);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//    
//    std::vector<float> interleavedData(nverts * 8);
//    for (int i = 0; i < nverts; i++) {
//        // Convert screen coordinates to clip space (-1 to 1)
//        float ndcX = (verts[i * 2] / viewportWidth) * 2.0f - 1.0f;
//        float ndcY = 1.0f - (verts[i * 2 + 1] / viewportHeight) * 2.0f;  // Flip Y
//        
//        interleavedData[i * 8 + 0] = ndcX;
//        interleavedData[i * 8 + 1] = ndcY;
//        interleavedData[i * 8 + 2] = tcoords[i * 2 + 0];
//        interleavedData[i * 8 + 3] = tcoords[i * 2 + 1];
//        
//        // Unpack color
//        unsigned int color = colors[i];
//        interleavedData[i * 8 + 4] = ((color >> 24) & 0xFF) / 255.0f;  // r
//        interleavedData[i * 8 + 5] = ((color >> 16) & 0xFF) / 255.0f;  // g
//        interleavedData[i * 8 + 6] = ((color >> 8) & 0xFF) / 255.0f;   // b
//        interleavedData[i * 8 + 7] = (color & 0xFF) / 255.0f;          // a
//    }
//    
//    glBindVertexArray(vao);
//    glBindBuffer(GL_ARRAY_BUFFER, vbo);
//    glBufferData(GL_ARRAY_BUFFER, interleavedData.size() * sizeof(float), interleavedData.data(), GL_STREAM_DRAW);
//    
//    glActiveTexture(GL_TEXTURE0);
//    glBindTexture(GL_TEXTURE_2D, fontTextureId);
//    
//    glDrawArrays(GL_TRIANGLES, 0, nverts);
//    
//    glDisable(GL_BLEND);
//    glBindVertexArray(0);
//    glUseProgram(0);
//}
//
//static void glfons__renderDelete(void* userPtr) {
//    if (fontTextureId) {
//        glDeleteTextures(1, &fontTextureId);
//        fontTextureId = 0;
//    }
//    if (vbo) {
//        glDeleteBuffers(1, &vbo);
//        vbo = 0;
//    }
//    if (vao) {
//        glDeleteVertexArrays(1, &vao);
//        vao = 0;
//    }
//}
//
//// Initialise Fontstash and OpenGL resources
//void initFontstash(int width, int height, const char* fontPath, int flags, GLuint shaderProgramID) {
//    std::cout << "Initializing Fontstash with dimensions: " << width << "x" << height << std::endl;
//    std::cout << "Using shader program: " << shaderProgramID << std::endl;
//
//    if (!glIsProgram(shaderProgramID)) {
//        std::cerr << "Shader program is invalid!" << std::endl;
//        return;
//    }
//
//    if (!glfwInit()) {
//        throw std::runtime_error("Failed to initialize GLFW");
//    }
//
//    textShaderID = shaderProgramID;
//
//    // Verify shader program
//    if (!glIsProgram(textShaderID)) {
//        throw std::runtime_error("Invalid shader program ID");
//    }
//
//    FONSparams params = {};
//    params.width = width;
//    params.height = height;
//    params.renderCreate = glfons__renderCreate;
//    params.renderResize = glfons__renderResize;
//    params.renderUpdate = glfons__renderUpdate;
//    params.renderDraw = glfons__renderDraw;
//    params.renderDelete = glfons__renderDelete;
//    params.userPtr = nullptr;
//
//    fs = fonsCreateInternal(&params);
//    if (!fs) {
//        throw std::runtime_error("Failed to initialize Fontstash");
//    }
//
//    // Load and verify font
//    int font = fonsAddFont(fs, "sans", fontPath);
//    if (font == FONS_INVALID) {
//        throw std::runtime_error("Failed to load font");
//    }
//    std::cout << "Font loaded successfully with ID: " << font << std::endl;
//
//    // Setup shader uniforms
//    glUseProgram(shaderProgramID);
//
//    // Set texture unit
//    GLint fontTexLoc = glGetUniformLocation(shaderProgramID, "fontTex");
//    if (fontTexLoc != -1) {
//        glUniform1i(fontTexLoc, 0);  // Use texture unit 0
//    }
//
//    // Make sure we have proper texture parameters
//    glBindTexture(GL_TEXTURE_2D, fontTextureId);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//
//    // Print initial texture state
//    std::cout << "Font texture ID: " << fontTextureId << std::endl;
//}
//
//// Delete Fontstash and OpenGL resources
//void deleteFontstash() 
//{
//    if (fs) {
//        fonsDeleteInternal(fs);
//        fs = nullptr;
//    }
//    if (vao) {
//        glDeleteVertexArrays(1, &vao);s
//        vao = 0;
//    }
//    if (vbo) {
//        glDeleteBuffers(1, &vbo);
//        vbo = 0;
//    }
//    if (fontTextureId) {
//        glDeleteTextures(1, &fontTextureId);
//        fontTextureId = 0;
//    }
//}
//
//
//void renderText(const char* text, float x, float y, int fontSize, unsigned int color) {
//    if (!fs) return;
//
//    // Convert from normalized (-1 to 1) to screen coordinates
//    GLint viewport[4];
//    glGetIntegerv(GL_VIEWPORT, viewport);
//    float screenX = (x + 1.0f) * viewport[2] * 0.5f;
//    float screenY = (1.0f - y) * viewport[3] * 0.5f;  // Flip Y coordinate
//
//    fonsSetFont(fs, 0);
//    fonsSetSize(fs, fontSize);
//    fonsSetColor(fs, color);
//
//    std::cout << "Screen coordinates: " << screenX << ", " << screenY << "\n";
//    fonsDrawText(fs, screenX, screenY, text, nullptr);
//}




#include <vector>

struct GLFONScontext {
	GLuint tex;
	GLuint VAO, VBO = 0;
	int width, height;
};
typedef struct GLFONScontext GLFONScontext;

static int glfons__renderCreate(void* userPtr, int width, int height)
{
	GLFONScontext* gl = (GLFONScontext*)userPtr;
	// Create may be called multiple times, delete existing texture.
	if (gl->tex != 0) {
		glDeleteTextures(1, &gl->tex);
		gl->tex = 0;
	}
	glGenTextures(1, &gl->tex);
	if (!gl->tex) return 0;
	gl->width = width;
	gl->height = height;
	glBindTexture(GL_TEXTURE_2D, gl->tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, gl->width, gl->height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	return 1;
}

static int glfons__renderResize(void* userPtr, int width, int height)
{
	// Reuse create to resize too.
	return glfons__renderCreate(userPtr, width, height);
}

static void glfons__renderUpdate(void* userPtr, int* rect, const unsigned char* data) {
	GLFONScontext* gl = (GLFONScontext*)userPtr;
	int w = rect[2] - rect[0];
	int h = rect[3] - rect[1];

	if (gl->tex == 0) return;

	// Save the current pixel store states
	GLint unpackAlignment, rowLength, skipPixels, skipRows;
	glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpackAlignment);
	glGetIntegerv(GL_UNPACK_ROW_LENGTH, &rowLength);
	glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &skipPixels);
	glGetIntegerv(GL_UNPACK_SKIP_ROWS, &skipRows);

	// Modify pixel store states for this operation
	glBindTexture(GL_TEXTURE_2D, gl->tex);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, gl->width);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, rect[0]);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, rect[1]);

	// Update texture subregion
	glTexSubImage2D(GL_TEXTURE_2D, 0, rect[0], rect[1], w, h, GL_ALPHA, GL_UNSIGNED_BYTE, data);

	// Restore the saved pixel store states
	glPixelStorei(GL_UNPACK_ALIGNMENT, unpackAlignment);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, rowLength);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, skipPixels);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, skipRows);
}


static void glfons__renderDraw(void* userPtr, const float* verts, const float* tcoords, const unsigned int* colors, int nverts) {
	GLFONScontext* gl = (GLFONScontext*)userPtr;

	if (gl->tex == 0) return;

	// Bind the font texture
	glBindTexture(GL_TEXTURE_2D, gl->tex);

	// Generate and bind a Vertex Array Object (VAO)
	GLuint VAO, VBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);

	// Combine all vertex attributes into a single interleaved array
	std::vector<float> vertexData;
	for (int i = 0; i < nverts; ++i) {
		// Add position (2 floats)
		vertexData.push_back(verts[i * 2 + 0]);
		vertexData.push_back(verts[i * 2 + 1]);
		// Add texture coordinates (2 floats)
		vertexData.push_back(tcoords[i * 2 + 0]);
		vertexData.push_back(tcoords[i * 2 + 1]);
		// Add colour (4 unsigned bytes, converted to floats)
		unsigned int color = colors[i];
		vertexData.push_back(((color >> 24) & 0xFF) / 255.0f); // Red
		vertexData.push_back(((color >> 16) & 0xFF) / 255.0f); // Green
		vertexData.push_back(((color >> 8) & 0xFF) / 255.0f);  // Blue
		vertexData.push_back((color & 0xFF) / 255.0f);         // Alpha
	}

	// Upload data to the GPU
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_DYNAMIC_DRAW);

	// Set up vertex attribute pointers
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);            // Position
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(2 * sizeof(float))); // TexCoords
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(4 * sizeof(float))); // Colours
	glEnableVertexAttribArray(2);

	// Draw the triangles
	glDrawArrays(GL_TRIANGLES, 0, nverts);

	// Clean up
	glBindVertexArray(0);
	glDeleteBuffers(1, &VBO);
	glDeleteVertexArrays(1, &VAO);
}

static void glfons__renderDelete(void* userPtr)
{
	GLFONScontext* gl = (GLFONScontext*)userPtr;
	if (gl->tex != 0)
		glDeleteTextures(1, &gl->tex);
	gl->tex = 0;
	free(gl);
}


FONScontext* openGLFonsCreate(int width, int height, int flags)
{
	FONSparams params;
	GLFONScontext* gl;

	gl = (GLFONScontext*)malloc(sizeof(GLFONScontext));
	if (gl == NULL) goto error;
	memset(gl, 0, sizeof(GLFONScontext));

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
	if (gl != NULL) free(gl);
	return NULL;
}

void openGLFonsDelete(FONScontext* ctx)
{
	fonsDeleteInternal(ctx);
}




//unsigned int glfonsRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
//{
//	return (r) | (g << 8) | (b << 16) | (a << 24);
//}

