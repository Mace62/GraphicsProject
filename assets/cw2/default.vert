#version 430

// Input data
// The layout( location = N ) syntax allows us to specify attribute indices directly in the shader. This
// avoids having to call glBindAttribLocation() when creating the shader program object. See
// https://www.khronos.org/opengl/wiki/Layout Qualifier (GLSL)
// for more information.
//
// Note: the indices that we specify here must match the ones that we set up in the vertex array object.
layout( location = 0 ) in vec3 iPosition;
layout( location = 1 ) in vec3 iColor;
layout( location = 2 ) in vec3 iNormal;
layout( location = 3 ) in vec2 iTexCoord;

layout( location = 0 ) uniform mat4 uProjCameraWorld;
layout( location = 1 ) uniform mat3 uNormalMatrix;




// Output attributes
// Output attributes are passed from the vertex shader, interpolated across the triangle/primitive, and then
// passed into the fragment shader. By default, output attributes are matched by name.
out vec3 v2fColor; // v2f = vertex to fragment

// Passing normal points to the frag shader
out vec3 v2fNormal;

// Output for texture co-ords
out vec2 v2fTexCoord;


// Each shader has a main() method. In the vertex shader, the main() method is run for each vertex that
// is processed.
void main()
{
	// Copy input color to the output color attribute.
	v2fColor = iColor;

	// Apply normalisation and pass as output
	v2fNormal = normalize(uNormalMatrix * iNormal);


	// Copy position to the built-in gl Position attribute
	// This attribute is the clip space position (homogeneous vertex position), which is always a vec4. For
	// now, we set z and w to zero and one, respectively. (z = zero is in the middle of the OpenGL clip space.
	// w = one, since this is a point and not a direction.)
	gl_Position = uProjCameraWorld * vec4( iPosition, 1.0 );

	// Just passing the texture co-ords through to the next stage in rendering pipeline
	// No transformations required
	v2fTexCoord = iTexCoord;

}