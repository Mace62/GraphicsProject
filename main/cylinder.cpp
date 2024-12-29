#include "cylinder.hpp"

#include <numbers>

SimpleMeshData make_cylinder(
    bool aCapped,
    std::size_t aSubdivs,
    Vec3f aColor,
    Mat44f aPreTransform,
    Vec3f aKa,
    Vec3f aKd,
    Vec3f aKs,
    float aNs,
    Vec3f aKe
) 
{
    SimpleMeshData data{};

    float prevY = std::cos(0.f);
    float prevZ = std::sin(0.f);

    // Precompute the normal matrix (3x3 inverse-transpose submatrix of aPreTransform)
    Mat33f const N = mat44_to_mat33(transpose(invert(aPreTransform)));

    for (std::size_t i = 0; i < aSubdivs; ++i)
    {
        float const angle = (i + 1) / float(aSubdivs) * 2.f * std::numbers::pi_v<float>;

        float y = std::cos(angle);
        float z = std::sin(angle);

        // Two triangles(= 3 * 2 positions) create one segment of the cylinder's shell.
        // Generate positions and normals for the shell
        data.positions.emplace_back(Vec3f{ 0.f, prevY, prevZ });
        data.normals.emplace_back(normalize(N * Vec3f{ 0.f, prevY, prevZ }));

        data.positions.emplace_back(Vec3f{ 0.f, y, z });
        data.normals.emplace_back(normalize(N * Vec3f{ 0.f, y, z }));

        data.positions.emplace_back(Vec3f{ 1.f, prevY, prevZ });
        data.normals.emplace_back(normalize(N * Vec3f{ 0.f, prevY, prevZ }));

        data.positions.emplace_back(Vec3f{ 0.f, y, z });
        data.normals.emplace_back(normalize(N * Vec3f{ 0.f, y, z }));

        data.positions.emplace_back(Vec3f{ 1.f, y, z });
        data.normals.emplace_back(normalize(N * Vec3f{ 0.f, y, z }));

        data.positions.emplace_back(Vec3f{ 1.f, prevY, prevZ });
        data.normals.emplace_back(normalize(N * Vec3f{ 0.f, prevY, prevZ }));

        // Add caps if needed
        if (aCapped)
        {
            // Cap at x = 0
            data.positions.emplace_back(Vec3f{ 0.f, prevY, prevZ });
            data.normals.emplace_back(normalize(N * Vec3f{ -1.f, 0.f, 0.f }));

            data.positions.emplace_back(Vec3f{ 0.f, y, z });
            data.normals.emplace_back(normalize(N * Vec3f{ -1.f, 0.f, 0.f }));

            data.positions.emplace_back(Vec3f{ 0.f, 0.f, 0.f });
            data.normals.emplace_back(normalize(N * Vec3f{ -1.f, 0.f, 0.f }));

            // Cap at x = 1
            data.positions.emplace_back(Vec3f{ 1.f, 0.f, 0.f });
            data.normals.emplace_back(normalize(N * Vec3f{ 1.f, 0.f, 0.f }));

            data.positions.emplace_back(Vec3f{ 1.f, y, z });
            data.normals.emplace_back(normalize(N * Vec3f{ 1.f, 0.f, 0.f }));

            data.positions.emplace_back(Vec3f{ 1.f, prevY, prevZ });
            data.normals.emplace_back(normalize(N * Vec3f{ 1.f, 0.f, 0.f }));
        }

        prevY = y;
        prevZ = z;
    }

    // Transform positions by aPreTransform
    for (auto& p : data.positions)
    {
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
