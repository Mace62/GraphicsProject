#ifndef PARTICLE_HPP
#define PARTICLE_HPP

#include <vector>
#include <glad/glad.h>

#include "../vmlib/vec3.hpp"
#include "../vmlib/vec4.hpp"
#include "../vmlib/mat44.hpp"

// Particle structure
struct Particle {
    Vec3f position;   // World-space position
    Vec3f velocity;   // Velocity vector
    float lifetime;   // Remaining lifetime (in seconds)
    float size;       // Size of the particle
    Vec4f color;      // Colour with alpha (RGBA)
};

constexpr int MAX_PARTICLES = 1000000;
//std::vector<Particle> particles;

void emitParticle(std::vector<Particle>& particles, const Vec4f& enginePosition, const Vec4f& engineDirection, const Mat44f& model2world);

void updateParticles(float deltaTime, std::vector<Particle>& particles);

void setupParticleSystem();

void renderParticles(const std::vector<Particle>& particles, GLuint shaderProgram, GLuint texture, Mat44f viewProjection);

#endif // PARTICLE_HPP
