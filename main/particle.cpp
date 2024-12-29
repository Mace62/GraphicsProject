#include "particle.hpp"

#include <random>

#include "../vmlib/vec4.hpp"

void emitParticle(Particle& particle, const Vec3f& enginePosition, const Vec3f& engineDirection) 
{
    
    particle.position = enginePosition + Vec3f(
        randomFloat(-0.1f, 0.1f),  // Random offset in X
        randomFloat(-0.1f, 0.1f),  // Random offset in Y
        randomFloat(-0.1f, 0.1f)); // Random offset in Z

    particle.velocity = Vec3f(
        randomFloat(-1.0f, 1.0f),
        randomFloat(-1.0f, 1.0f),
        randomFloat(-1.0f, 1.0f)); // Random velocity

    particle.lifetime = randomFloat(1.0f, 3.0f); // Lifetime between 1-3 seconds
    particle.size = randomFloat(0.1f, 0.3f);     // Random size
    particle.color = Vec4f(1.0f, 0.5f, 0.0f, 1.0f); // Orange exhaust
}


void updateParticles(float deltaTime) 
{
    for (auto& particle : particles) 
    {
        if (particle.lifetime > 0.0f) 
        {
            particle.position += particle.velocity * deltaTime;
            particle.lifetime -= deltaTime;

            // Fade out as lifetime decreases
            particle.color.a = std::max(0.0f, particle.lifetime / 3.0f);
        }
        else
        {

        }
    }
}
