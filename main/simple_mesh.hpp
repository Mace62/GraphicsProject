#ifndef SIMPLE_MESH_HPP_C6B749D6_C83B_434C_9E58_F05FC27FEFC9
#define SIMPLE_MESH_HPP_C6B749D6_C83B_434C_9E58_F05FC27FEFC9

#include <glad/glad.h>

#include <vector>

#include "../vmlib/vec3.hpp"
#include "../vmlib/vec2.hpp"

struct SimpleMeshData {
    std::vector<Vec3f> positions;  // Vertex positions
    std::vector<Vec3f> normals;    // Vertex normals
    std::vector<Vec3f> colors;     // Material color (diffuse)
    std::vector<Vec2f> texcoords;  // Texture coordinates
    std::vector<Vec3f> Ka;         // Ambient reflectivity
    std::vector<Vec3f> Kd;         // Diffuse reflectivity
    std::vector<Vec3f> Ks;         // Specular reflectivity
    std::vector<float> Ns;         // Shininess
    std::vector<Vec3f> Ke;         // Emission
    Vec2f mins;                    // Min tex coord for normalization
    Vec2f diffs;                   // Tex coord diff for normalization
    bool isTextureSupplied = false;
};


SimpleMeshData concatenate( SimpleMeshData, SimpleMeshData const& );


GLuint create_vao( SimpleMeshData const& );

#endif // SIMPLE_MESH_HPP_C6B749D6_C83B_434C_9E58_F05FC27FEFC9
