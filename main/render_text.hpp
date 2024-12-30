#ifndef RENDER_TEXT_HPP
#define RENDER_TEXT_HPP

#include <fontstash.h>
#include <glad/glad.h>

FONScontext* glfonsCreate(int width, int height, int flags, GLuint shaderProgramID);
void glfonsDelete(FONScontext* ctx);

unsigned int glfonsRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
void renderText(FONScontext* fsContext, const char* text, float x, float y, float fontSize, unsigned int color, int font);

void glfonsUpdateWindowSize(FONScontext* ctx, int width, int height);

#endif // RENDER_TEXT_HPP
