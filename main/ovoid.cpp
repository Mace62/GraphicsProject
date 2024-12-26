#include "ovoid.hpp"

#include "../vmlib/mat33.hpp"

#include <numbers>


SimpleMeshData make_truncated_ovoid(
    std::size_t aCircleSubdivs,
    std::size_t aHeightSubdivs,
    float verticalScale,
    float topCutoff,
    float bottomCutoff,
    Vec3f aColor,
    Mat44f aPreTransform,
    Vec3f aKa,
    Vec3f aKd,
    Vec3f aKs,
    float aNs,
    Vec3f aKe
) {
    SimpleMeshData data{};

    // Precompute the normal matrix
    Mat33f const N = mat44_to_mat33(transpose(invert(aPreTransform)));

    // Calculate phi (vertical) angles with cutoffs
    float phiStart = bottomCutoff * std::numbers::pi_v<float>;
    float phiEnd = (1.0f - topCutoff) * std::numbers::pi_v<float>;
    float phiStep = (phiEnd - phiStart) / float(aHeightSubdivs);

    // Calculate theta (horizontal) step
    float thetaStep = 2.0f * std::numbers::pi_v<float> / float(aCircleSubdivs);

    // Generate vertices and triangles
    for (std::size_t phi_idx = 0; phi_idx < aHeightSubdivs; ++phi_idx) {
        float phi1 = phiStart + phi_idx * phiStep;
        float phi2 = phi1 + phiStep;

        for (std::size_t theta_idx = 0; theta_idx < aCircleSubdivs; ++theta_idx) {
            float theta1 = theta_idx * thetaStep;
            float theta2 = theta1 + thetaStep;

            // Calculate vertices for current quad
            // Note: y is scaled by verticalScale to create elongation
            auto calcVertex = [verticalScale](float phi, float theta) -> Vec3f {
                return Vec3f{
                    std::sin(phi) * std::cos(theta),
                    verticalScale * std::cos(phi),
                    std::sin(phi) * std::sin(theta)
                };
            };

            auto calcNormal = [verticalScale, &N](float phi, float theta) -> Vec3f {
                // For an ovoid, we need to adjust the normal based on the vertical scaling
                Vec3f normal{
                    std::sin(phi) * std::cos(theta),
                    std::cos(phi) / verticalScale, // Adjust normal for vertical scaling
                    std::sin(phi) * std::sin(theta)
                };
                return normalize(N * normalize(normal));
            };

            // Calculate four corners of the quad
            Vec3f v1 = calcVertex(phi1, theta1);
            Vec3f v2 = calcVertex(phi1, theta2);
            Vec3f v3 = calcVertex(phi2, theta1);
            Vec3f v4 = calcVertex(phi2, theta2);

            // Calculate normals for each vertex
            Vec3f n1 = calcNormal(phi1, theta1);
            Vec3f n2 = calcNormal(phi1, theta2);
            Vec3f n3 = calcNormal(phi2, theta1);
            Vec3f n4 = calcNormal(phi2, theta2);

            // First triangle of quad
            data.positions.insert(data.positions.end(), { v1, v2, v3 });
            data.normals.insert(data.normals.end(), { n1, n2, n3 });

            // Second triangle of quad
            data.positions.insert(data.positions.end(), { v2, v4, v3 });
            data.normals.insert(data.normals.end(), { n2, n4, n3 });
        }
    }

    // Transform positions
    for (auto& p : data.positions) {
        Vec4f p4{ p.x, p.y, p.z, 1.f };
        Vec4f t = aPreTransform * p4;
        t /= t.w;
        p = Vec3f{ t.x, t.y, t.z };
    }

    // Add colors
    data.colors.assign(data.positions.size(), aColor);

    // Add material properties
    data.Ka.assign(data.positions.size(), aKa);
    data.Kd.assign(data.positions.size(), aKd);
    data.Ks.assign(data.positions.size(), aKs);
    data.Ns.assign(data.positions.size(), aNs);
    data.Ke.assign(data.positions.size(), aKe);


    return data;
}