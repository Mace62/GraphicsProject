#include "loadobj.hpp"

#include <rapidobj/rapidobj.hpp>
#include <iostream>

#include "../support/error.hpp"
#include "../vmlib/mat33.hpp"
#include "../vmlib/vec2.hpp"


SimpleMeshData load_wavefront_obj(char const* aPath, bool isTextureSupplied, Mat44f aPreTransform)
{
    // Load the OBJ file
    auto result = rapidobj::ParseFile(aPath);
    if (result.error)
        throw Error("Unable to load OBJ file '%s': %s", aPath, result.error.code.message().c_str());

    // Triangulate non-triangle faces
    rapidobj::Triangulate(result);

    SimpleMeshData ret;

    // Variables to store the smallest and largest x and z for tex coords (looking to normalise)
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minZ = std::numeric_limits<float>::max();
    float maxZ = std::numeric_limits<float>::lowest();

    // Calculate normal transformation matrix
    Mat33f const N = mat44_to_mat33(transpose(invert(aPreTransform)));

    // Iterate through the shapes and load the necessary data
    for (auto const& shape : result.shapes)
    {
        // For each face in the shape
        for (std::size_t i = 0; i < shape.mesh.indices.size(); ++i)
        {
            auto const& idx = shape.mesh.indices[i];

            // Add vertex position
            Vec3f position = mat44_to_mat33(aPreTransform) * Vec3f {
                result.attributes.positions[idx.position_index * 3 + 0],
                    result.attributes.positions[idx.position_index * 3 + 1],
                    result.attributes.positions[idx.position_index * 3 + 2]
            };
            ret.positions.emplace_back(position);

            // Update min and max values for position's x and z components
            minX = std::min(minX, position.x);
            maxX = std::max(maxX, position.x);
            minZ = std::min(minZ, position.z);
            maxZ = std::max(maxZ, position.z);

            // Add normal if present
            if (idx.normal_index >= 0) {
                ret.normals.emplace_back(N * Vec3f{
                    result.attributes.normals[idx.normal_index * 3 + 0],
                    result.attributes.normals[idx.normal_index * 3 + 1],
                    result.attributes.normals[idx.normal_index * 3 + 2]
                    });
            }

            // Add texture coordinates if present
            if (idx.texcoord_index >= 0) {
                float texX = result.attributes.texcoords[idx.texcoord_index * 2 + 0];
                float texZ = result.attributes.texcoords[idx.texcoord_index * 2 + 1];
                ret.texcoords.emplace_back(Vec2f{ texX, texZ });
            }

            

            // Get material for current face
            auto const& mat = result.materials[shape.mesh.material_ids[i / 3]];

            // Add color (all white)
            ret.colors.emplace_back(Vec3f{
                mat.ambient[0],
                mat.ambient[1],
                mat.ambient[2]
               });

            // Add ambient (Ka), diffuse (Kd), specular (Ks), and emission (Ke)
            // Ambient saved as colours
            ret.Ka.emplace_back(Vec3f{
                mat.ambient[0],
                mat.ambient[1],
                mat.ambient[2]
            });

            ret.Kd.emplace_back(Vec3f{
                mat.diffuse[0],
                mat.diffuse[1],
                mat.diffuse[2]
            });

            ret.Ks.emplace_back(Vec3f{
                mat.specular[0],
                mat.specular[1],
                mat.specular[2]
            });

            ret.Ke.emplace_back(Vec3f{
                mat.emission[0],
                mat.emission[1],
                mat.emission[2]
            });

            // Add shininess (Ns)
            ret.Ns.push_back(mat.shininess);
        }
    }

    // Compute the differences between max and min values for tex coordinates
    float diffX = maxX - minX;
    float diffZ = maxZ - minZ;

    // Output the computed differences
    std::cout << "maxX: " << maxX << ", minX: " << minX << std::endl;
    std::cout << "maxZ: " << maxZ << ", minZ: " << minZ << std::endl;
    std::cout << "diffX: " << diffX << ", diffZ: " << diffZ << std::endl;

    // Save min and diffs for texturing recalculation to upper left corner
    ret.mins = Vec2f{ minX, minZ };
    ret.diffs = Vec2f{ diffX, diffZ };

    std::cout << ret.positions.size() << std::endl;

    ret.isTextureSupplied = isTextureSupplied;

    // Transform positions by aPreTransform
    for (auto& p : ret.positions)
    {
        Vec4f p4{ p.x, p.y, p.z, 1.f };
        Vec4f t = aPreTransform * p4;
        t /= t.w;

        p = Vec3f{ t.x, t.y, t.z };
    }

    return ret;
}


