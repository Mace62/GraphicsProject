#ifndef CONE_HPP_CB812C27_5E45_4ED9_9A7F_D66774954C29
#define CONE_HPP_CB812C27_5E45_4ED9_9A7F_D66774954C29

#include <vector>

#include <cstdlib>

#include "simple_mesh.hpp"

#include "../vmlib/vec3.hpp"
#include "../vmlib/mat44.hpp"
#include "../vmlib/mat33.hpp"

SimpleMeshData make_cone(
    bool aCapped = true,
    std::size_t aSubdivs = 16,
    Vec3f aColor = { 1.f, 1.f, 1.f },
    Mat44f aPreTransform = kIdentity44f,
    Vec3f aKa = { 0.2f, 0.2f, 0.2f },    // Ambient reflectivity
    Vec3f aKd = { 0.5f, 0.5f, 0.5f },    // Diffuse reflectivity
    Vec3f aKs = { 0.2f, 0.2f, 0.2f },    // Specular reflectivity
    float aNs = 10.f,                    // Shininess
    Vec3f aKe = { 0.f, 0.f, 0.f }        // Emission
);

#endif // CONE_HPP_CB812C27_5E45_4ED9_9A7F_D66774954C29
