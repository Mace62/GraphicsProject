
#include <array>

#include "../vmlib/vec3.hpp"

struct Particle {
    Vec3f position;   // World-space position
    Vec3f velocity;   // Velocity vector
    float lifetime;       // Remaining lifetime
    float size;           // Size of the particle
    Vec4f color;      // Colour with alpha
};

constexpr int MAX_PARTICLES = 1000;
std::array<Particle, MAX_PARTICLES> particles;

void emitParticle(Particle& particle, const Vec3f& enginePosition, const Vec3f& engineDirection);