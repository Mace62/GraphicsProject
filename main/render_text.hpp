#ifndef RENDER_TEXT_HPP
#define RENDER_TEXT_HPP

#include <fontstash.h>
#include <glad/glad.h>

extern FONScontext* fs;  // Fontstash context

//void initFontstash(int width, int height, const char* fontPath, int flags, GLuint shaderProgramID);
//void deleteFontstash();
//void renderText(const char* text, float x, float y, int fontSize, unsigned int color);

FONScontext* openGLFonsCreate(int width, int height, int flags);
void openGLFonsDelete(FONScontext* ctx);


#endif // RENDER_TEXT_HPP
