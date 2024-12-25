#ifndef TRUNCATED_OVOID_HPP
#define TRUNCATED_OVOID_HPP

#include "simple_mesh.hpp"
#include "../vmlib/mat44.hpp"
#include "../vmlib/vec3.hpp"

#include <cstddef> // For std::size_t

/**
 * @brief Generates a truncated ovoid (ellipsoid-like) mesh with specified top and bottom cutoffs.
 *
 * @param aCircleSubdivs Number of subdivisions around the circumference.
 * @param aHeightSubdivs Number of subdivisions along the height.
 * @param verticalScale Scaling factor for the height (elongation of the ovoid).
 * @param topCutoff Fraction of the ovoid to cut off from the top (0.0 - 1.0).
 * @param bottomCutoff Fraction of the ovoid to cut off from the bottom (0.0 - 1.0).
 * @param aColor Colour to apply to the mesh vertices.
 * @param aPreTransform A pre-transform matrix to apply to the entire ovoid.
 * @return SimpleMeshData A mesh containing the positions, normals, and colours for the truncated ovoid.
 */
SimpleMeshData make_truncated_ovoid(
    std::size_t aCircleSubdivs,
    std::size_t aHeightSubdivs,
    float verticalScale,
    float topCutoff,
    float bottomCutoff,
    Vec3f aColor,
    Mat44f aPreTransform
);

#endif // TRUNCATED_OVOID_HPP
