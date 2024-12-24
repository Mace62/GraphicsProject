#include "loadobj.hpp"

#include <rapidobj/rapidobj.hpp>
#include <iostream>

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

    // Variables to store the smallest and largest x and z for tex coords (looking to normalise)
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minZ = std::numeric_limits<float>::max();
    float maxZ = std::numeric_limits<float>::lowest();

    // Calculate normal transformation matrix
    Mat33f const N = mat44_to_mat33(transpose(invert(aPreTransform)));

    for (auto const& shape : result.shapes)
    {
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
                // Use normal from OBJ file
                ret.normals.emplace_back(N * Vec3f{
                    result.attributes.normals[idx.normal_index * 3 + 0],
                    result.attributes.normals[idx.normal_index * 3 + 1],
                    result.attributes.normals[idx.normal_index * 3 + 2]
                    });
            }

            // Add texture coordinates if present
            if (idx.texcoord_index >= 0) {
                // Assuming result.attributes.texcoords contains the texture coordinates in the form of an array
                float texX = result.attributes.texcoords[idx.texcoord_index * 2 + 0];
                float texZ = result.attributes.texcoords[idx.texcoord_index * 2 + 1];

                // Save the texture coordinates
                ret.texcoords.emplace_back(Vec2f{ texX, texZ });

                //// Update the min and max values for x and z
                //minX = std::min(minX, texX);
                //maxX = std::max(maxX, texX);
                //minZ = std::min(minZ, texZ);
                //maxZ = std::max(maxZ, texZ);
            }

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

    // Compute the differences between max and min values
    float diffX = maxX - minX;
    float diffZ = maxZ - minZ;

    // Output the computed differences
    std::cout << "maxX: " << maxX << ", minX: " << minX << std::endl;
    std::cout << "maxz: " << maxZ << ", minZ: " << minZ << std::endl;
    std::cout << "diffX: " << diffX << ", diffZ: " << diffZ << std::endl;

    // Set each texture coordinate to range [-1, 0]
    for (auto& texCoord : ret.texcoords) 
    {
        // Output before values of x and z
        //std::cout << "Before - x: " << texCoord.x << ", z: " << texCoord.y << std::endl;

        // Normalize x and z values to the range [-1, 0]
        texCoord.x = texCoord.x - 1.0f;
        texCoord.y = texCoord.y - 1.0f;

        // Output after values of x and z
        //std::cout << "After - x: " << texCoord.x << ", z: " << texCoord.y << std::endl;
    }

    std::cout << ret.positions.size() << std::endl;
    return ret;
}

