#version 430
// Input attributes
// These should match the outputs from the vertex shader.
in vec3 v2fColor;
in vec3 v2fNormal;
in vec2 v2fTexCoord;

// Uniform data
// For now, we use the “register style” uniforms. Similar to the vertex shader inputs, the
// layout( location = N )
// syntax allows us to specify uniform locations in the shader (which avoids the need to query them later,
// via glGetUniformLocation()). Again, see
// https://www.khronos.org/opengl/wiki/Layout Qualifier (GLSL)
// for more information
//
// Warning: This feature was added in OpenGL 4.3 / GLSL 430! (Apple: check if Apple supports the
// GL ARB explicit uniform location extension, which provides the same functionality.)
// layout( location = 0 ) uniform vec3 uBaseColor;

layout( location = 2 ) uniform vec3 uLightDir; // should be normalized! (uLightDir) = 1
layout( location = 3 ) uniform vec3 uLightDiffuse;
layout( location = 4 ) uniform vec3 uSceneAmbient;
layout( binding = 0 ) uniform sampler2D uTexture;


// Fragment shader outputs
// For now, we have a single output, a RGB (=vec3) color. This is written to the color attachment with
// index 0 (the location value below). For the moment, we are drawing the default framebuffer, which has
// only one color attachment: color attachment 0, which is the window’s back buffer.
layout( location = 0 ) out vec3 oColor;


void main()
{
	vec3 normal = normalize(v2fNormal);

	float nDotL = max( 0.0, dot( normal, uLightDir ) );
	oColor = (uSceneAmbient + nDotL * uLightDiffuse) * v2fColor;

	oColor = texture( uTexture, v2fTexCoord ).rgb;		// For texturing only


	// oColor = normal;		// Use this as a debug method to visualise normals as colours 

	// oColor = vec3(v2fTexCoord[0], v2fTexCoord[1], 0.0);		// Use this as a debug to visualise texture co-ords as colours
	
	// oColor = v2fColor;
}


