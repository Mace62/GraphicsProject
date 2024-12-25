#include "simple_mesh.hpp"

SimpleMeshData concatenate(SimpleMeshData aM, const SimpleMeshData& aN) {
    // Concatenate vertex positions
    aM.positions.insert(aM.positions.end(), aN.positions.begin(), aN.positions.end());

    // Concatenate normals
    aM.normals.insert(aM.normals.end(), aN.normals.begin(), aN.normals.end());

    // Concatenate colours
    aM.colors.insert(aM.colors.end(), aN.colors.begin(), aN.colors.end());

    // Concatenate texture coordinates
    aM.texcoords.insert(aM.texcoords.end(), aN.texcoords.begin(), aN.texcoords.end());

    // Concatenate material properties
    aM.Ka.insert(aM.Ka.end(), aN.Ka.begin(), aN.Ka.end());
    aM.Kd.insert(aM.Kd.end(), aN.Kd.begin(), aN.Kd.end());
    aM.Ks.insert(aM.Ks.end(), aN.Ks.begin(), aN.Ks.end());
    aM.Ns.insert(aM.Ns.end(), aN.Ns.begin(), aN.Ns.end());
    aM.Ke.insert(aM.Ke.end(), aN.Ke.begin(), aN.Ke.end());

    // Handle mins and diffs
    // Combine mins and diffs by calculating the bounding range for texture coordinates
    aM.mins.x = std::min(aM.mins.x, aN.mins.x);
    aM.mins.y = std::min(aM.mins.y, aN.mins.y);
    aM.diffs.x = std::max(aM.diffs.x, aN.diffs.x);
    aM.diffs.y = std::max(aM.diffs.y, aN.diffs.y);

    return aM;
}




GLuint create_vao(SimpleMeshData const& aMeshData)
{
    GLuint positionVBO = 0;
    glGenBuffers(1, &positionVBO);
    glBindBuffer(GL_ARRAY_BUFFER, positionVBO);
    glBufferData(GL_ARRAY_BUFFER, aMeshData.positions.size() * sizeof(Vec3f), aMeshData.positions.data(), GL_STATIC_DRAW);

    GLuint colorVBO = 1;
    glGenBuffers(1, &colorVBO);
    glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
    glBufferData(GL_ARRAY_BUFFER, aMeshData.colors.size() * sizeof(Vec3f), aMeshData.colors.data(), GL_STATIC_DRAW);

    GLuint normalVBO = 2;
    glGenBuffers(1, &normalVBO);
    glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
    glBufferData(GL_ARRAY_BUFFER, aMeshData.normals.size() * sizeof(Vec3f), aMeshData.normals.data(), GL_STATIC_DRAW);

    GLuint textureVBO = 3;
    glGenBuffers(1, &textureVBO);
    glBindBuffer(GL_ARRAY_BUFFER, textureVBO);
    glBufferData(GL_ARRAY_BUFFER, aMeshData.texcoords.size() * sizeof(Vec2f), aMeshData.texcoords.data(), GL_STATIC_DRAW);

    // VBOs for material properties
    GLuint KaVBO = 4;
    glGenBuffers(1, &KaVBO);
    glBindBuffer(GL_ARRAY_BUFFER, KaVBO);
    glBufferData(GL_ARRAY_BUFFER, aMeshData.Ka.size() * sizeof(Vec3f), aMeshData.Ka.data(), GL_STATIC_DRAW);

    GLuint KdVBO = 5;
    glGenBuffers(1, &KdVBO);
    glBindBuffer(GL_ARRAY_BUFFER, KdVBO);
    glBufferData(GL_ARRAY_BUFFER, aMeshData.Kd.size() * sizeof(Vec3f), aMeshData.Kd.data(), GL_STATIC_DRAW);

    GLuint KsVBO = 6;
    glGenBuffers(1, &KsVBO);
    glBindBuffer(GL_ARRAY_BUFFER, KsVBO);
    glBufferData(GL_ARRAY_BUFFER, aMeshData.Ks.size() * sizeof(Vec3f), aMeshData.Ks.data(), GL_STATIC_DRAW);

    GLuint NsVBO = 7;
    glGenBuffers(1, &NsVBO);
    glBindBuffer(GL_ARRAY_BUFFER, NsVBO);
    glBufferData(GL_ARRAY_BUFFER, aMeshData.Ns.size() * sizeof(float), aMeshData.Ns.data(), GL_STATIC_DRAW);

    GLuint KeVBO = 8;
    glGenBuffers(1, &KeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, KeVBO);
    glBufferData(GL_ARRAY_BUFFER, aMeshData.Ke.size() * sizeof(Vec3f), aMeshData.Ke.data(), GL_STATIC_DRAW);

    // Create VAO
    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Position
    glBindBuffer(GL_ARRAY_BUFFER, positionVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    // Color
    glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);

    // Normal
    glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(2);

    // Texture coordinates
    glBindBuffer(GL_ARRAY_BUFFER, textureVBO);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(3);

    // Ka (Ambience reflectivity)
    glBindBuffer(GL_ARRAY_BUFFER, KaVBO);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(5);

    // Kd (Diffuse reflectivity)
    glBindBuffer(GL_ARRAY_BUFFER, KdVBO);
    glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(4);

    // Ks (Specular reflectivity)
    glBindBuffer(GL_ARRAY_BUFFER, KsVBO);
    glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(5);

    // Ns (Shininess)
    glBindBuffer(GL_ARRAY_BUFFER, NsVBO);
    glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(6);

    // Ke (Emission)
    glBindBuffer(GL_ARRAY_BUFFER, KeVBO);
    glVertexAttribPointer(8, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(7);

    // Reset state
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Clean up buffers
    glDeleteBuffers(1, &colorVBO);
    glDeleteBuffers(1, &positionVBO);
    glDeleteBuffers(1, &normalVBO);
    glDeleteBuffers(1, &textureVBO);
    glDeleteBuffers(1, &KaVBO);
    glDeleteBuffers(1, &KdVBO);
    glDeleteBuffers(1, &KsVBO);
    glDeleteBuffers(1, &NsVBO);
    glDeleteBuffers(1, &KeVBO);

    return vao;
}

