#ifndef TRIANGLE_PRISM_LOADER
#define TRIANGLE_PRISM_LOADER

#include <vector>

#include <cstdlib>

#include "simple_mesh.hpp"

#include "../vmlib/vec3.hpp"
#include "../vmlib/mat44.hpp"


SimpleMeshData make_triangle_based_prism(
    bool centre_prism = false,
    Vec2f p1 = { 0.f, 0.f }, Vec2f p2 = { 0.f, 0.f }, Vec2f p3 = { 0.f, 0.f }, // Flat x and y coordinates only
    float depth = 1,                        // Determines z value
    Vec3f aColor = { 1.f, 1.f, 1.f },
    Mat44f aPreTransform = kIdentity44f,
    Vec3f aKa = { 0.2f, 0.2f, 0.2f },       // Ambient reflectivity
    Vec3f aKd = { 0.5f, 0.0f, 0.0f },       // Diffuse reflectivity (red for the wing)
    Vec3f aKs = { 0.2f, 0.2f, 0.2f },       // Specular reflectivity
    float aNs = 15.f,                       // Shininess
    Vec3f aKe = { 0.f, 0.f, 0.f }           // Emission
);


#endif // TRIANGLE_PRISM_LOADER
