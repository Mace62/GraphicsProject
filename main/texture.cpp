#include "texture.hpp"

#include <cassert>

#include <stb_image.h>

#include "../support/error.hpp"

GLuint load_texture_2d(char const* aPath)
{
	assert(aPath);
	
	// Load image first
	// This may fail (e.g., image does not exist), so there's no point in
	// allocating OpenGL resources ahead of time.
	//stbi_set_flip_vertically_on_load(true);
	
	int w, h, channels;
	stbi_uc * ptr = stbi_load(aPath, &w, &h, &channels, 4);
	if(!ptr)
		throw Error("Unable to load image %s\n", aPath);
	
	// Generate texture object and initialize texture with image
	GLuint tex = 0;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, ptr);
	
	stbi_image_free(ptr);
	
	// Generate mipmap hierarchy
	glGenerateMipmap(GL_TEXTURE_2D);
	
	// Configure texture
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);		// GL_CLAMP_TO_EDGE will clamp texture to edge
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, 16.f);
	
	return tex;
}

GLuint load_texture_2d_with_alpha(char const* aPath)
{
	assert(aPath);

	// Load the image (alpha included)
	int w, h, channels;
	// Load image with 4 channels (RGBA)
	stbi_uc* ptr = stbi_load(aPath, &w, &h, &channels, 4);
	if (!ptr)
		throw Error("Unable to load image %s\n", aPath);

	// Generate texture object and initialize texture with image
	GLuint tex = 0;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	// Load the texture with RGBA channels (which includes alpha)
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, ptr);

	stbi_image_free(ptr);

	// Generate mipmap hierarchy
	glGenerateMipmap(GL_TEXTURE_2D);

	// Configure texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, 16.f);

	return tex;
}
