#ifndef TRUNCATED_OVOID_HPP
#define TRUNCATED_OVOID_HPP

#include "simple_mesh.hpp"
#include "../vmlib/mat44.hpp"
#include "../vmlib/vec3.hpp"

#include <cstddef> // For std::size_t

// Generates a truncated ovoid (ellipsoid-like) mesh with specified top and bottom cutoffs.

SimpleMeshData make_truncated_ovoid(
    std::size_t aCircleSubdivs,
    std::size_t aHeightSubdivs,
    float verticalScale,
    float topCutoff,
    float bottomCutoff,
    Vec3f aColor = { 1.f, 1.f, 1.f },
    Mat44f aPreTransform = kIdentity44f,
    Vec3f aKa = { 0.2f, 0.2f, 0.2f },       // Ambient reflectivity
    Vec3f aKd = { 0.8f, 0.8f, 0.8f },       // Diffuse reflectivity (silver)
    Vec3f aKs = { 0.6f, 0.6f, 0.6f },       // Specular reflectivity (shiny metal)
    float aNs = 50.f,                       // Shininess
    Vec3f aKe = { 0.f, 0.f, 0.f }           // Emission
);

#endif // TRUNCATED_OVOID_HPP
