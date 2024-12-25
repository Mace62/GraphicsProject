#include "triangle_prism.hpp"

#include "../vmlib/mat33.hpp"


SimpleMeshData make_triangle_based_prism(
    bool centre_prism,
    Vec2f p1, Vec2f p2, Vec2f p3,
    float depth,
    Vec3f aColor,
    Mat44f aPreTransform
) {
    SimpleMeshData data{};

    // Calculate offset for centering (only in Y and Z, as per requirement)
    float offsetY = 0.0f;
    float offsetZ = 0.0f;
    if (centre_prism) {
        float minY = std::min({ p1.x, p2.x, p3.x });
        float maxY = std::max({ p1.x, p2.x, p3.x });
        float minZ = std::min({ p1.y, p2.y, p3.y });
        float maxZ = std::max({ p1.y, p2.y, p3.y });
        offsetY = -(maxY + minY) / 2.0f;
        offsetZ = -(maxZ + minZ) / 2.0f;
    }

    // Calculate depth offset
    float offsetX = centre_prism ? -depth / 2.0f : 0.0f;

    // Precompute the normal matrix (3x3 inverse-transpose submatrix of aPreTransform)
    Mat33f const N = mat44_to_mat33(transpose(invert(aPreTransform)));

    // Create vertices for front and back faces
    Vec3f v1_front{ offsetX, p1.x + offsetY, p1.y + offsetZ };
    Vec3f v2_front{ offsetX, p2.x + offsetY, p2.y + offsetZ };
    Vec3f v3_front{ offsetX, p3.x + offsetY, p3.y + offsetZ };
    Vec3f v1_back{ offsetX + depth, p1.x + offsetY, p1.y + offsetZ };
    Vec3f v2_back{ offsetX + depth, p2.x + offsetY, p2.y + offsetZ };
    Vec3f v3_back{ offsetX + depth, p3.x + offsetY, p3.y + offsetZ };

    // Calculate face normals
    Vec3f front_normal = normalize(N * Vec3f{ -1.0f, 0.0f, 0.0f });
    Vec3f back_normal = normalize(N * Vec3f{ 1.0f, 0.0f, 0.0f });

    // Front face
    data.positions.insert(data.positions.end(), { v1_front, v2_front, v3_front });
    data.normals.insert(data.normals.end(), { front_normal, front_normal, front_normal });

    // Back face
    data.positions.insert(data.positions.end(), { v1_back, v3_back, v2_back }); // Note reversed order for correct winding
    data.normals.insert(data.normals.end(), { back_normal, back_normal, back_normal });

    // Side faces
    // Calculate normal for each side face using cross product
    Vec3f side1_normal = normalize(N * cross(v2_front - v1_front, Vec3f{ 1.0f, 0.0f, 0.0f }));
    Vec3f side2_normal = normalize(N * cross(v3_front - v2_front, Vec3f{ 1.0f, 0.0f, 0.0f }));
    Vec3f side3_normal = normalize(N * cross(v1_front - v3_front, Vec3f{ 1.0f, 0.0f, 0.0f }));

    // Side 1
    data.positions.insert(data.positions.end(), { v1_front, v1_back, v2_front });
    data.positions.insert(data.positions.end(), { v2_front, v1_back, v2_back });
    data.normals.insert(data.normals.end(), { side1_normal, side1_normal, side1_normal });
    data.normals.insert(data.normals.end(), { side1_normal, side1_normal, side1_normal });

    // Side 2
    data.positions.insert(data.positions.end(), { v2_front, v2_back, v3_front });
    data.positions.insert(data.positions.end(), { v3_front, v2_back, v3_back });
    data.normals.insert(data.normals.end(), { side2_normal, side2_normal, side2_normal });
    data.normals.insert(data.normals.end(), { side2_normal, side2_normal, side2_normal });

    // Side 3
    data.positions.insert(data.positions.end(), { v3_front, v3_back, v1_front });
    data.positions.insert(data.positions.end(), { v1_front, v3_back, v1_back });
    data.normals.insert(data.normals.end(), { side3_normal, side3_normal, side3_normal });
    data.normals.insert(data.normals.end(), { side3_normal, side3_normal, side3_normal });

    // Transform positions by aPreTransform
    for (auto& p : data.positions) {
        Vec4f p4{ p.x, p.y, p.z, 1.f };
        Vec4f t = aPreTransform * p4;
        t /= t.w;
        p = Vec3f{ t.x, t.y, t.z };
    }

    // Add colors
    data.colors.assign(data.positions.size(), aColor);

    return data;
}