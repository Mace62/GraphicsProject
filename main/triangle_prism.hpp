#ifndef TRIANGLE_PRISM_LOADER
#define TRIANGLE_PRISM_LOADER

#include <vector>

#include <cstdlib>

#include "simple_mesh.hpp"

#include "../vmlib/vec3.hpp"
#include "../vmlib/mat44.hpp"


SimpleMeshData make_triangle_based_prism(
	bool centre_prism = false,
	Vec2f p1 = { 0.f, 0.f }, Vec2f p2 = { 0.f, 0.f }, Vec2f p3 = { 0.f, 0.f },		// Flat x and y co-ords only
	float depth = 1,						// Determines z value
	Vec3f aColor = { 1.f, 1.f, 1.f },
	Mat44f aPreTransform = kIdentity44f
);

#endif // TRIANGLE_PRISM_LOADER
