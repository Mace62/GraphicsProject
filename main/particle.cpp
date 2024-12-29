#include "particle.hpp"

#include <random>

#include "../vmlib/vec4.hpp"
#include "../vmlib/mat44.hpp"

void emitParticle(std::vector<Particle>& particles, const Vec4f& enginePosition, const Vec4f& engineDirection, const Mat44f& model2world)
{
    // Transform position
    Vec4f engineWorldPos = model2world * enginePosition;

    // Transform direction by taking only the rotational component of the matrix
    Vec4f dirToTransform = engineDirection;
    dirToTransform.w = 0.0f;
    Vec4f engineWorldDir = model2world * dirToTransform;
    engineWorldDir.w = 0.0f;

    // Convert to Vec3
    Vec3f engPos = { engineWorldPos.x, engineWorldPos.y, engineWorldPos.z };
    Vec3f engDir = normalize(Vec3f{ engineWorldDir.x, engineWorldDir.y, engineWorldDir.z });

    // Assign particle values
    Particle newParticle;
    newParticle.position = engPos;

    // Calculate random direction offset 
    Vec3f randomOffset = {
        (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.5f,
        (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.5f,
        (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.5f
    };

    newParticle.velocity = engDir * 5.0f + randomOffset;

    newParticle.lifetime = 5.0f;
    newParticle.size = 100.0f;
    newParticle.color = { 1.0f, 0.5f, 0.1f, 1.0f };

    particles.push_back(newParticle);
}

void updateParticles(float deltaTime, std::vector<Particle>& particles)
{
    for (auto& particle : particles)
    {
        if (particle.lifetime > 0.0f)
        {
            // Update particle's position and fade
            particle.position += particle.velocity * deltaTime;
            particle.lifetime -= deltaTime;

            // Fade out as lifetime decreases
            particle.color.w = std::max(0.0f, particle.lifetime / 3.0f);
        }
    }

    // Remove expired particles
    particles.erase(
        std::remove_if(
            particles.begin(), particles.end(),
            [](const Particle& p) { return p.lifetime <= 0.0f; }
        ),
        particles.end()
    );
}


// OpenGL handles
GLuint particleVAO, particleVBO = 0;

void setupParticleSystem() 
{
    // Generate buffers
    glGenVertexArrays(1, &particleVAO);
    glGenBuffers(1, &particleVBO);

    glBindVertexArray(particleVAO);

    // Allocate memory for the particle VBO
    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * sizeof(Particle), nullptr, GL_DYNAMIC_DRAW);

    // Enable position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, position));
    glEnableVertexAttribArray(0);

    // Enable colour attribute
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, color));
    glEnableVertexAttribArray(1);

    // Enable size attribute
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, size));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    //return particleVAO;
}

void renderParticles(const std::vector<Particle>& particles, GLuint shaderProgram, GLuint texture, Mat44f viewProjection) 
{
    // Use particle shader program
    glUseProgram(shaderProgram);
    glBlendFunc(GL_ONE, GL_SRC_ALPHA);
    glDepthMask(GL_FALSE);  // Disable depth writing for transparent objects
    glEnable(GL_PROGRAM_POINT_SIZE);  // Enable controlling point size via shaders

    // Pass uniform data
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uViewProjection"), 1, GL_TRUE, viewProjection.v);

    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(glGetUniformLocation(shaderProgram, "uTexture"), 0);

    // Update particle buffer
    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, particles.size() * sizeof(Particle), particles.data());

    // Draw particles
    glBindVertexArray(particleVAO);
    glDrawArrays(GL_POINTS, 0, particles.size());
    glBindVertexArray(0);

    // Re-enable depth writing for next solid object
    glDepthMask(GL_TRUE);   
    // Disable point size control from shader (restore default)
    glDisable(GL_PROGRAM_POINT_SIZE);
}


