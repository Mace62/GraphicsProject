#include "loadobj.hpp"

#include <rapidobj/rapidobj.hpp>

#include "../support/error.hpp"
#include "../vmlib/mat33.hpp"
#include "../vmlib/vec2.hpp"


SimpleMeshData load_wavefront_obj(char const* aPath, Mat44f aPreTransform)
{
    // Load the OBJ file
    auto result = rapidobj::ParseFile(aPath);
    if (result.error)
        throw Error("Unable to load OBJ file '%s': %s", aPath, result.error.code.message().c_str());

    // Triangulate non-triangle faces
    rapidobj::Triangulate(result);

    SimpleMeshData ret;

    // Calculate normal transformation matrix
    Mat33f const N = mat44_to_mat33(transpose(invert(aPreTransform)));

    for (auto const& shape : result.shapes)
    {
        for (std::size_t i = 0; i < shape.mesh.indices.size(); ++i)
        {
            auto const& idx = shape.mesh.indices[i];

            // Add vertex position
            ret.positions.emplace_back(mat44_to_mat33(aPreTransform) * Vec3f{
                result.attributes.positions[idx.position_index * 3 + 0],
                result.attributes.positions[idx.position_index * 3 + 1],
                result.attributes.positions[idx.position_index * 3 + 2]
                });

            // Add normal if present
            if (idx.normal_index >= 0) {
                // Use normal from OBJ file
                ret.normals.emplace_back(N * Vec3f{
                    result.attributes.normals[idx.normal_index * 3 + 0],
                    result.attributes.normals[idx.normal_index * 3 + 1],
                    result.attributes.normals[idx.normal_index * 3 + 2]
                    });
            }
           

            // Add texture coordinates if present
            //if (idx.texcoord_index >= 0) {
            //    ret.texcoords.emplace_back(Vec2f{
            //        result.attributes.texcoords[idx.texcoord_index * 2 + 0],
            //        result.attributes.texcoords[idx.texcoord_index * 2 + 1]
            //        });
            //}
            //else {
            //    // Add default UV coordinates if none present
            //    ret.texcoords.emplace_back(Vec2f{ 0.0f, 0.0f });
            //}

            // Get material for current face
            auto const& mat = result.materials[shape.mesh.material_ids[i / 3]];

            // Add color from material
            ret.colors.emplace_back(Vec3f{
                mat.ambient[0],
                mat.ambient[1],
                mat.ambient[2]
            });
        }
    }

    return ret;
}

