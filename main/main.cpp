#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <numbers>
#include <typeinfo>
#include <stdexcept>

#include <cstdio>
#include <cstdlib>

#include <filesystem>
#include <string>
#include <iostream>
#include <chrono>       // For timing logic

#include "../support/error.hpp"
#include "../support/program.hpp"
#include "../support/checkpoint.hpp"
#include "../support/debug_output.hpp"

#include "../vmlib/vec4.hpp"
#include "../vmlib/mat44.hpp"
#include "../vmlib/mat33.hpp"

#include "defaults.hpp"
#include "loadobj.hpp"
#include "texture.hpp"
#include "spaceship.hpp"
#include "particle.hpp"
//#include "render_text.hpp"

#define M_PI 3.14159265358979323846

#if defined(_WIN32) // alternative: #if defined(_MSC_VER)
extern "C"
{
    __declspec(dllexport) unsigned long NvOptimusEnablement = 1;
    __declspec(dllexport) unsigned long AmdPowerXpressRequestHighPerformance = 1; // untested
}
#endif

// ------------------- Assets & Constants --------------------
const std::string DIR_PATH = std::filesystem::current_path().string();

const std::string LANGERSO_OBJ_ASSET_PATH        = DIR_PATH + "/assets/cw2/langerso.obj";
const std::string LANGERSO_TEXTURE_ASSET_PATH    = DIR_PATH + "/assets/cw2/L3211E-4k.jpg";
const std::string LAUNCHPAD_OBJ_ASSET_PATH       = DIR_PATH + "/assets/cw2/landingpad.obj";
const std::string PARTICLE_TEXTURE_ASSET_PATH    = DIR_PATH + "/assets/cw2/explosion.png";

static constexpr int MAX_POINT_LIGHTS = 3;
static constexpr Vec3f rocketStartPos = { 0.0f, 0.0f, 0.0f };

namespace
{
    // --------------- Window Title & Movement Constants -------------
    constexpr char const* kWindowTitle = "COMP3811 - CW2";

    constexpr float kMovementPerSecond_ = 5.f;     // movement units per second
    constexpr float kMouseSensitivity_  = 0.01f;   // radians per pixel

    constexpr float rocketAcceleration_ = 0.1f;

    using Clock    = std::chrono::high_resolution_clock;
    using Secondsf = std::chrono::duration<float>;

    // --------------- Camera Mode ---------------
    enum class CameraMode {
        FREE = 0,
        CHASE,
        GROUND
    };

    // --------------- Program State ---------------
    struct State_
    {
        

        // Shaders
        ShaderProgram* prog = nullptr;
        ShaderProgram* particleShader = nullptr;

        // Split-screen
        bool isSplitScreen = false;

        // We have two camera modes, one for each subview
        CameraMode cameraMode1 = CameraMode::FREE;
        CameraMode cameraMode2 = CameraMode::CHASE;

        // Each camera has its own control
        struct CamCtrl_
        {
            float FAST_SPEED_MULT  = 2.f;
            float SLOW_SPEED_MULT  = 0.1f;
            float NORMAL_SPEED_MULT= 0.5f;

            bool movingForward = false;
            bool movingBack    = false;
            bool movingLeft    = false;
            bool movingRight   = false;
            bool movingUp      = false;
            bool movingDown    = false;

            // Orientation
            Vec4f position = { 0.0f, 5.0f, 0.0f, 1.0f };
            Vec4f forward  = { 0.0f, 0.0f, -1.0f, 0.0f };
            Vec4f right    = { 1.0f, 0.0f, 0.0f, 0.0f };
            Vec4f up       = { 0.0f, 1.0f, 0.0f, 0.0f };

            bool cameraActive    = false;
            bool actionZoomIn    = false;
            bool actionZoomOut   = false;

            float phi   = 0.0f;  // yaw
            float theta = 0.0f;  // pitch
            float radius= 10.f;
            float speed_multiplier= NORMAL_SPEED_MULT;

            float lastX=0.f, lastY=0.f, lastTheta=0.f;
        };

        CamCtrl_ cam1;  // First camera controls
        CamCtrl_ cam2;  // Second camera controls

        // -------------- Point Lights --------------
        struct PointLight {
            Vec3f position;  
            float padding1;  
            Vec3f color;     
            float padding2;  
            Vec3f normals;   
            float radius;    
        };

        struct PointLightBlock {
            PointLight lights[MAX_POINT_LIGHTS];
        };

        // -------------- Rocket State --------------
        struct rcktCtrl_ {
            Vec3f position = rocketStartPos;
            Vec3f velocity = { 0.f, 0.f, 0.f };
            Mat44f model2worldRocket = kIdentity44f;
            float acceleration = rocketAcceleration_;
            float time = 0.f;
            bool isMoving = false;
            float pitch=0.f, yaw=0.f;

            Vec4f enginePosition = { 0.f, 0.f, 0.f, 1.f };
            Vec4f engineDirection = { 0.f, 0.f, 0.f, 1.f };

            // Particle storage
            std::vector<Particle> particles;
            float particleTimer = 0.f;

            Mat44f translationMatrix;
            Mat44f rotation;

            void reset() {
                model2worldRocket = kIdentity44f;
                position = rocketStartPos;
                velocity = { 0.0f, 0.0f, 0.0f };
                acceleration = rocketAcceleration_;
                time=0.f;
                isMoving=false;
                pitch=0.f;
                particleTimer = 0.f;
                particles.clear();
            }
        } rcktCtrl;

        float chaseDistance = 1.0f;            
        Vec3f groundCameraPos = { -5.f, 1.0f, 0.f };
    };

    // ------------------ Function Declarations ------------------
    void glfw_callback_error_(int, char const*);
    void glfw_callback_key_(GLFWwindow*, int, int, int, int);
    void glfw_callback_motion_(GLFWwindow*, double, double);
    void mouse_button_callback(GLFWwindow*, int, int, int);

    void updateCamera(State_::CamCtrl_& camera, float dt);
    void updateRocket(State_::rcktCtrl_& rocket, float dt);

    Mat44f compute_view_matrix_for_camera(const State_::CamCtrl_& camCtrl,
                                          CameraMode mode,
                                          const State_& state);

    GLuint setPointLights(State_::PointLight pointLights[MAX_POINT_LIGHTS], SimpleMeshData rocketPos);

    void updatePointLights(Mat44f rocketPosition, SimpleMeshData rocketData, State_::PointLight pointLights[MAX_POINT_LIGHTS]);

    void updatePointLightUBO(GLuint pointLightUBO,
                             State_::PointLight pointLights[MAX_POINT_LIGHTS]);

    // This function: draws the entire scene for one camera's view+proj
    void renderScene(State_& state,
                     const Mat44f& view,
                     const Mat44f& projection,
                     // Below are references to the various VAOs & meshes:
                     GLuint langersoVao, const SimpleMeshData& langersoMesh, GLuint langersoTextureId, size_t langersoCount,
                     GLuint rocketVao,   const SimpleMeshData& rocketMesh,   size_t rocketCount,
                     GLuint launchpadVao, const SimpleMeshData& launchpadMesh, size_t launchpadCount,
                     GLuint particleTextureId);

    // RAII-like helpers
    struct GLFWCleanupHelper
    {
        ~GLFWCleanupHelper() { glfwTerminate(); }
    };
    struct GLFWWindowDeleter
    {
        ~GLFWWindowDeleter() { if(window) glfwDestroyWindow(window); }
        GLFWwindow* window;
    };

    void glfw_callback_error_(int aErrNum, char const* aErrDesc)
    {
        std::fprintf(stderr, "GLFW error: %s (%d)\n", aErrDesc, aErrNum);
    }

    void glfw_callback_key_(GLFWwindow* aWindow, int aKey, int, int aAction, int mods)
    {
        if (GLFW_KEY_ESCAPE == aKey && GLFW_PRESS == aAction) {
            glfwSetWindowShouldClose(aWindow, GLFW_TRUE);
            return;
        }

        if (auto* state = static_cast<State_*>(glfwGetWindowUserPointer(aWindow))) {
            // Press V to toggle split screen:
            if (aKey == GLFW_KEY_V && aAction == GLFW_PRESS) {
                state->isSplitScreen = !state->isSplitScreen;
            }

            // Press C to cycle camera 1's mode:
            if (aKey == GLFW_KEY_C && (mods & GLFW_MOD_SHIFT) == 0 && aAction == GLFW_PRESS) {
                switch (state->cameraMode1) {
                case CameraMode::FREE:   state->cameraMode1 = CameraMode::CHASE;   break;
                case CameraMode::CHASE:  state->cameraMode1 = CameraMode::GROUND; break;
                case CameraMode::GROUND: state->cameraMode1 = CameraMode::FREE;   break;
                }
            }

            // Press Shift + C to cycle camera 2's mode:
            if (aKey == GLFW_KEY_C && (mods & GLFW_MOD_SHIFT) && aAction == GLFW_PRESS) {
                switch (state->cameraMode2) {
                //case CameraMode::FREE:   state->cameraMode2 = CameraMode::CHASE;   break;
                case CameraMode::CHASE:  state->cameraMode2 = CameraMode::GROUND; break;
                case CameraMode::GROUND: state->cameraMode2 = CameraMode::CHASE;   break;
                }
            }

            // Start rocket animation with 'F'
            if (GLFW_KEY_F == aKey && GLFW_PRESS == aAction) {
                state->rcktCtrl.isMoving = !state->rcktCtrl.isMoving;
            }
            // R-key reloads shaders.
            if (GLFW_KEY_R == aKey && GLFW_PRESS == aAction) {
                if (state->prog) {
                    state->rcktCtrl.reset();
                    try {
                        state->prog->reload();
                        std::fprintf(stderr, "Shaders reloaded and recompiled.\n");
                    }
                    catch (std::exception const& eErr) {
                        std::fprintf(stderr, "Error when reloading shader:\n");
                        std::fprintf(stderr, "%s\n", eErr.what());
                        std::fprintf(stderr, "Keeping old shader.\n");
                    }
                }
            }

            // Handle WASD keys for cam1:
            if (GLFW_KEY_W == aKey) {
                state->cam1.movingForward = (aAction != GLFW_RELEASE);
            }
            if (GLFW_KEY_S == aKey) {
                state->cam1.movingBack = (aAction != GLFW_RELEASE);
            }
            if (GLFW_KEY_A == aKey) {
                state->cam1.movingLeft = (aAction != GLFW_RELEASE);
            }
            if (GLFW_KEY_D == aKey) {
                state->cam1.movingRight = (aAction != GLFW_RELEASE);
            }
            if (GLFW_KEY_E == aKey) {
                state->cam1.movingUp = (aAction != GLFW_RELEASE);
            }
            if (GLFW_KEY_Q == aKey) {
                state->cam1.movingDown = (aAction != GLFW_RELEASE);
            }

            // SHIFT speeds up camera, CONTROL slows it down
            if (aKey == GLFW_KEY_LEFT_SHIFT || aKey == GLFW_KEY_RIGHT_SHIFT) {
                if (aAction == GLFW_PRESS) {
                    state->cam1.speed_multiplier = state->cam1.FAST_SPEED_MULT;
                }
                else if (aAction == GLFW_RELEASE) {
                    state->cam1.speed_multiplier = state->cam1.NORMAL_SPEED_MULT;
                }
            }

            if (aKey == GLFW_KEY_LEFT_CONTROL || aKey == GLFW_KEY_RIGHT_CONTROL) {
                if (aAction == GLFW_PRESS) {
                    state->cam1.speed_multiplier = state->cam1.SLOW_SPEED_MULT;
                }
                else if (aAction == GLFW_RELEASE) {
                    state->cam1.speed_multiplier = state->cam1.NORMAL_SPEED_MULT;
                }
            }
        }
    }

    void glfw_callback_motion_(GLFWwindow* aWindow, double aX, double aY)
    {
        if (auto* state = static_cast<State_*>(glfwGetWindowUserPointer(aWindow)))
        {
            // Only rotate camera1 if it's active
            if (state->cam1.cameraActive)
            {
                float dx = float(aX - state->cam1.lastX);
                float dy = float(aY - state->cam1.lastY);

                state->cam1.phi += dx * kMouseSensitivity_;
                state->cam1.theta += dy * kMouseSensitivity_;

                // Limit pitch to +/- 90°, same as before
                if (state->cam1.theta > std::numbers::pi_v<float> / 2.f)
                    state->cam1.theta = state->cam1.lastTheta;
                else if (state->cam1.theta < -std::numbers::pi_v<float> / 2.f)
                    state->cam1.theta = state->cam1.lastTheta;
            }

            state->cam1.lastX = float(aX);
            state->cam1.lastY = float(aY);
            state->cam1.lastTheta = state->cam1.theta;
        }
    }

    void mouse_button_callback(GLFWwindow* aWindow, int button, int action, int /*mods*/)
    {
        if (auto* state = static_cast<State_*>(glfwGetWindowUserPointer(aWindow)))
        {
            if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
            {
                // Toggle camera1's "active" state
                state->cam1.cameraActive = !state->cam1.cameraActive;

                if (state->cam1.cameraActive) {
                    // Hide and lock cursor to window center
                    glfwSetInputMode(aWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                }
                else {
                    // Normal cursor
                    glfwSetInputMode(aWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                }
            }
        }
    }


    // Function to update camera position based on movement
    void updateCamera(State_::CamCtrl_& camera, float dt)
    {
        // Calculate movement speed
        float moveSpeed = kMovementPerSecond_ * dt * camera.speed_multiplier;

        // Update forward and right vectors based on phi (yaw) and theta (pitch)
        camera.forward = Vec4f{
            std::sin(camera.phi) * std::cos(camera.theta),
            -std::sin(camera.theta),
            -std::cos(camera.phi) * std::cos(camera.theta),
            0.0f
        };

        // Debugger statements
        //std::cout << "phi: " << camera.phi << "		sin(phi) = " << sin(camera.phi) << "		cos(phi) = " << cos(camera.phi) << std::endl;
        //std::cout << "theta: " << camera.theta << "		sin(theta) = " << sin(camera.theta) << "		cos(theta) = " << cos(camera.theta) << "\n" << std::endl;

        // Calculate right vector by crossing forward with world up
        camera.right = cross(camera.forward, Vec4f{ 0.0f, 1.0f, 0.0f, 0.0f });
        camera.right = normalize(camera.right);

        // Calculate actual up vector
        camera.up = cross(camera.right, camera.forward);

        // Apply movement
        Vec4f movement{ 0.0f, 0.0f, 0.0f, 0.0f };

        if (camera.movingForward)
            movement = movement + camera.forward * moveSpeed;
        if (camera.movingBack)
            movement = movement - camera.forward * moveSpeed;
        if (camera.movingRight)
            movement = movement + camera.right * moveSpeed;
        if (camera.movingLeft)
            movement = movement - camera.right * moveSpeed;
        if (camera.movingUp)
            movement = movement + camera.up * moveSpeed;
        if (camera.movingDown)
            movement = movement - camera.up * moveSpeed;

        // Update position
        camera.position = camera.position + movement;

        /*std::cout << "Camera position: ("
            << camera.position.x << ", "
            << camera.position.y << ", "
            << camera.position.z << ", "
            << camera.position.w << ")\n";*/

    }

    GLuint setPointLights(State_::PointLight pointLights[MAX_POINT_LIGHTS], SimpleMeshData rocketPos)
    {
        // Update point light data with larger radius values
        pointLights[0].position = rocketPos.pointLightPos[0];
        pointLights[0].radius = 1.0f;        // Increased radius significantly
        pointLights[0].color = Vec3f{ 1.f, 0.f, 0.f }; // Red
        pointLights[0].normals = rocketPos.pointLightNorms[0];

        pointLights[1].position = rocketPos.pointLightPos[1];
        pointLights[1].radius = 1.0f;
        pointLights[1].color = Vec3f{ 0.f, 1.f, 0.f }; // Green
        pointLights[1].normals = rocketPos.pointLightNorms[1];

        pointLights[2].position = rocketPos.pointLightPos[2];
        pointLights[2].radius = 1.0f;
        pointLights[2].color = Vec3f{ 0.f, 0.f, 1.f }; // Blue
        pointLights[2].normals = rocketPos.pointLightNorms[2];

        for (size_t i = 0; i < MAX_POINT_LIGHTS; ++i)
        {
            std::cout << "Point Light " << i << " Position: ("
                << pointLights[i].position.x << ", "
                << pointLights[i].position.y << ", "
                << pointLights[i].position.z << ")\n";
        }

        // Create and setup UBO
        GLuint pointLightUBO;
        glGenBuffers(1, &pointLightUBO);
        glBindBuffer(GL_UNIFORM_BUFFER, pointLightUBO);

        // Allocate and initialize buffer in one step
        State_::PointLightBlock pointLightData;
        for (int i = 0; i < MAX_POINT_LIGHTS; ++i) {
            pointLightData.lights[i] = pointLights[i];
        }
        glBufferData(GL_UNIFORM_BUFFER, sizeof(State_::PointLightBlock), &pointLightData, GL_STATIC_DRAW);

        // Bind to uniform buffer binding point
        glBindBufferBase(GL_UNIFORM_BUFFER, 1, pointLightUBO);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        return pointLightUBO;
    }

    void updatePointLights(Mat44f rocketPosition, SimpleMeshData rocketData, State_::PointLight pointLights[MAX_POINT_LIGHTS])
    {
        Mat33f const N = mat44_to_mat33(transpose(invert(rocketPosition)));
        for (int i = 0; i < MAX_POINT_LIGHTS; ++i)
        {
            // Transform light positions with rocket matrix
            Vec4f transformedPos = rocketPosition * Vec4f{ rocketData.pointLightPos[i].x, rocketData.pointLightPos[i].y, rocketData.pointLightPos[i].z, 1.0f };
            Vec3f transformedNorm = normalize(N * rocketData.pointLightNorms[i]);

            pointLights[i].position = rocketData.pointLightPos[i];
            pointLights[i].normals = transformedNorm;

            //std::cout << "Before - Position: (" << rocketData.pointLightPos[i].x << ", " << rocketData.pointLightPos[i].y << ", " << rocketData.pointLightPos[i].z << ")" << ", Normal: (" << rocketData.pointLightNorms[i].x << ", " << rocketData.pointLightNorms[i].y << ", " << rocketData.pointLightNorms[i].z << ")\n"; std::cout << "After - Position: (" << transformedPos.x << ", " << transformedPos.y << ", " << transformedPos.z << ")" << ", Normal: (" << transformedNorm.x << ", " << transformedNorm.y << ", " << transformedNorm.z << ")\n";

            // Maintain other properties that were set in setPointLights
            if (i == 0) {
                pointLights[i].color = Vec3f{ 1.f, 0.f, 0.f }; // Red
            }
            else if (i == 1) {
                pointLights[i].color = Vec3f{ 0.f, 1.f, 0.f }; // Green
            }
            else {
                pointLights[i].color = Vec3f{ 0.f, 0.f, 1.f }; // Blue
            }
            pointLights[i].radius = 1.0f;
        }
    }

    void updatePointLightUBO(GLuint pointLightUBO, State_::PointLight pointLights[MAX_POINT_LIGHTS])
    {
        State_::PointLightBlock pointLightData;
        for (int i = 0; i < MAX_POINT_LIGHTS; ++i) {
            pointLightData.lights[i] = pointLights[i];
        }

        // Debugging
        /*for (size_t i = 0; i < MAX_POINT_LIGHTS; ++i)
        {
            std::cout << "Point Light " << i << " Position: ("
                << pointLights[i].position.x << ", "
                << pointLights[i].position.y << ", "
                << pointLights[i].position.z << ")\n";
        }*/


        // Update the buffer without recreating it
        glBindBuffer(GL_UNIFORM_BUFFER, pointLightUBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(State_::PointLightBlock), &pointLightData);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }


    Mat44f compute_view_matrix_for_camera(const State_::CamCtrl_& camCtrl, CameraMode mode, const State_& state)
    {
        switch (mode) {
        case CameraMode::FREE: {
            return make_look_at(
                camCtrl.position,
                camCtrl.position + camCtrl.forward,
                camCtrl.up
            );
        }

        // (2) Chase camera
        case CameraMode::CHASE:
        {
            // Chase from behind the rocket at a fixed distance
            auto const& rocketPos = state.rcktCtrl.position;

            Vec3f rocketForwardWS = { 0.f, 0.f, -1.f };

            // Position the chase camera behind and slightly above rocket
            Vec3f chaseCamPos = rocketPos - rocketForwardWS * state.chaseDistance + Vec3f{ 0.f, 1.f, 0.f };
            Vec4f chaseCamPos4 = { chaseCamPos.x, chaseCamPos.y, chaseCamPos.z, 1.f };

            // Look towards rocket
            Vec4f rocketPos4 = { rocketPos.x + 1.47f , rocketPos.y, rocketPos.z - 1.20f, 1.f };

            return make_look_at(
                chaseCamPos4,
                rocketPos4,
                Vec4f{ 0.f, 1.f, 0.f, 0.f }  // world-up
            );
        }

        // (3) Ground camera
        case CameraMode::GROUND:
        {
            // Always stay at groundCameraPos, look at rocket.
            auto const& rocketPos = state.rcktCtrl.position;
            Vec4f rocketPos4 = { rocketPos.x + 1.47f , rocketPos.y, rocketPos.z - 1.20f, 1.f };
            Vec4f groundPos4 = {
                state.groundCameraPos.x,
                state.groundCameraPos.y,
                state.groundCameraPos.z,
                1.f
            };

            return make_look_at(
                groundPos4,
                rocketPos4,
                Vec4f{ 0.f, 1.f, 0.f, 0.f }  // world-up
            );
        }
        }

        // Should never happen, but return identity if logic breaks:
        return kIdentity44f;
    }


    void updateRocket(State_::rcktCtrl_& rocket, float dt) {
        if (rocket.isMoving) {
            // Store previous position for direction calculation
            Vec3f previousPosition = rocket.position;

            rocket.time += dt;

            // Define the direction vector for the rocket's motion after 5 seconds
            Vec3f newDirection = normalize(Vec3f(3.0f, 1.0f, -3.5f)); // Example direction vector (change as needed)

            // Initialize the acceleration vector based on the time elapsed
            Vec3f accelerationVector;

            if (rocket.time <= 5.0f) {
                // For the first 5 seconds, only the y-component of acceleration is set to 2
                accelerationVector = normalize(Vec3f(0.0f, 1.0f, 0.0f));
            }
            else {
                // After 5 seconds, scale the direction vector by the desired acceleration magnitude
                accelerationVector = newDirection * rocket.acceleration;
            }

            // Update velocity with the calculated acceleration components
            rocket.velocity.x += accelerationVector.x * dt;
            rocket.velocity.y += accelerationVector.y * dt;
            rocket.velocity.z += accelerationVector.z * dt;

            // Update position based on the velocity
            rocket.position.x += rocket.velocity.x * dt;
            rocket.position.y += rocket.velocity.y * dt;
            rocket.position.z += rocket.velocity.z * dt;

            // Calculate the direction of motion
            Vec3f direction = {
                rocket.position.x - previousPosition.x,
                rocket.position.y - previousPosition.y,
                rocket.position.z - previousPosition.z
            };

            // Normalize the direction vector to prevent errors when calculating rotation
            if (length(direction) > 0.001f) {
                direction = normalize(direction);
            }

            // Assume the rocket moves primarily along the y-axis (forward direction)
            Vec3f rocketForward = Vec3f(0.0f, 1.0f, 0.0f);

            // Calculate pitch: angle between the forward direction and the direction vector
            float pitch = atan2(direction.z, sqrt(direction.x * direction.x + direction.y * direction.y));

            // Calculate yaw: angle in the x-y plane
            float yaw = atan2(direction.x, direction.y);

            // Debug pitch and yaw for verification
            /*printf("Pitch: %f radians\n", pitch);
            printf("Yaw: %f radians\n", yaw);*/

            // Create rotation matrices for pitch and yaw
            Mat44f rotationMatrixPitch = make_rotation_x(pitch); // Pitch around X-axis
            Mat44f rotationMatrixYaw = make_rotation_z(-yaw);     // Yaw around Z-axis

            // Combine rotations in the correct order (YPR)
            Mat44f rotationMatrix = rotationMatrixYaw * rotationMatrixPitch;

            // Translate the rocket to its current position
            Mat44f translationMatrix = make_translation(rocket.position);

            // Combine translation and rotation into the final model-to-world matrix
            rocket.model2worldRocket = translationMatrix * rotationMatrix * kIdentity44f;

            // Emit a particle every 0.002 seconds
            while (rocket.particleTimer >= 0.0002f)
            {
                // Emit a particle
                emitParticle(rocket.particles, rocket.enginePosition, rocket.engineDirection, rocket.model2worldRocket);

                // Reset timer (or subtract 0.002f to allow for continuous emission if multiple particles are to be emitted)
                rocket.particleTimer -= 0.0002f;

            }

            // Debug output for verification
            /*printf("Rocket Time: %f\n", rocket.time);
            printf("Position: (%f, %f, %f)\n", rocket.position.x, rocket.position.y, rocket.position.z);
            printf("Velocity: (%f, %f, %f)\n", rocket.velocity.x, rocket.velocity.y, rocket.velocity.z);
            printf("Direction: (%f, %f, %f)\n", direction.x, direction.y, direction.z);*/
        }
        else
            rocket.particleTimer = 1.f;
    }
} // end anonymous namespace


// ------------------ Main ------------------
int main() try
{
    // Initialize GLFW
    if (GLFW_TRUE != glfwInit())
    {
        char const* msg=nullptr;
        int ecode = glfwGetError(&msg);
        throw Error("glfwInit() failed with '%s' (%d)", msg, ecode);
    }
    GLFWCleanupHelper cleanupHelper;

    // Error callback
    glfwSetErrorCallback(&glfw_callback_error_);

    // Window hints
#   if !defined(__APPLE__)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
#   else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,1);
#   endif
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);

#   if !defined(NDEBUG)
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT,GL_TRUE);
#   endif

    // Create window
    GLFWwindow* window = glfwCreateWindow(1280, 720,
                                          kWindowTitle,
                                          nullptr,nullptr);
    if(!window)
    {
        char const* msg=nullptr;
        int ecode = glfwGetError(&msg);
        throw Error("glfwCreateWindow() failed with '%s' (%d)", msg, ecode);
    }
    GLFWWindowDeleter windowDeleter{window};

    // Set up user pointer
    State_ state{};
    glfwSetWindowUserPointer(window, &state);

    // Key & mouse callbacks
    glfwSetKeyCallback(window, &glfw_callback_key_);
    glfwSetCursorPosCallback(window, &glfw_callback_motion_);
    glfwSetMouseButtonCallback(window, &mouse_button_callback);

    // Make context current
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // vsync

    // Init GLAD
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        throw Error("Failed to initialize GLAD!");

    std::printf("RENDERER                   %s\n", glGetString(GL_RENDERER));
    std::printf("VENDOR                     %s\n", glGetString(GL_VENDOR));
    std::printf("VERSION                    %s\n", glGetString(GL_VERSION));
    std::printf("SHADING_LANGUAGE_VERSION   %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

#   if !defined(NDEBUG)
    setup_gl_debug_output();
#   endif

    OGL_CHECKPOINT_ALWAYS();

    // Global GL state
    glClearColor(0.2f,0.2f,0.2f,0.0f);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_DEPTH_TEST);
    // glEnable(GL_CULL_FACE);
    // glCullFace(GL_BACK);
    // glFrontFace(GL_CCW);
    glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_TRUE);

    // Framebuffer size
    int iwidth, iheight;
    glfwGetFramebufferSize(window, &iwidth, &iheight);
    glViewport(0,0,iwidth,iheight);

    // Load shaders
    ShaderProgram prog({
        {GL_VERTEX_SHADER,   "assets/cw2/default.vert"},
        {GL_FRAGMENT_SHADER, "assets/cw2/default.frag"}
    });
    state.prog = &prog;

    ShaderProgram particleShader({
        {GL_VERTEX_SHADER,   "assets/cw2/particle.vert"},
        {GL_FRAGMENT_SHADER, "assets/cw2/particle.frag"}
        });
    state.particleShader = &particleShader;

    // -------------- Load all meshes & textures --------------
    // Langerso
    auto langersoMesh = load_wavefront_obj(LANGERSO_OBJ_ASSET_PATH.c_str(), true);
    GLuint langersoVao = create_vao(langersoMesh);
    GLuint langersoTextureId = load_texture_2d(LANGERSO_TEXTURE_ASSET_PATH.c_str());
    size_t langersoVertexCount = langersoMesh.positions.size();

    // Launchpad
    auto launchpadMesh = load_wavefront_obj(
        LAUNCHPAD_OBJ_ASSET_PATH.c_str(),
        false,
        make_translation({ 2.f, 0.005f, -2.f}) * make_scaling(0.5f, 0.5f, 0.5f)
    );
    GLuint launchpadVao = create_vao(launchpadMesh);
    size_t launchpadVertexCount = launchpadMesh.positions.size();

    // Rocket
    auto rocketMesh = create_spaceship(
        32,
        {0.2f, 0.2f, 0.2f}, {0.8f, 0.2f, 0.2f}, // body & fin colors
        make_translation({2.f,0.15f,-2.f}) * make_scaling(0.05f,0.05f,0.05f),
        false
    );
    GLuint rocketVao = create_vao(rocketMesh);
    size_t rocketVertexCount = rocketMesh.positions.size();
    state.rcktCtrl.enginePosition = rocketMesh.engineLocation;
    state.rcktCtrl.engineDirection = rocketMesh.engineDirection;


    // Particles
    setupParticleSystem();
    GLuint particleTextureId = load_texture_2d_with_alpha(PARTICLE_TEXTURE_ASSET_PATH.c_str());

    // -------------- Set up lights --------------
    State_::PointLight pointLights[MAX_POINT_LIGHTS];
    GLuint pointLightUBO = setPointLights(pointLights, rocketMesh);

    OGL_CHECKPOINT_ALWAYS();

    // -------------- Timing variables --------------
    auto last = Clock::now();

    // Main loop
    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Check for window resizing
        int w,h;
        glfwGetFramebufferSize(window, &w, &h);
        if (w<=0 || h<=0)
        {
            // Minimized? wait
            do {
                glfwWaitEvents();
                glfwGetFramebufferSize(window,&w,&h);
            } while(w<=0 || h<=0);
        }
        glViewport(0,0,w,h);

        // Compute dt
        auto now   = Clock::now();
        float dt   = std::chrono::duration_cast<Secondsf>(now - last).count();
        last       = now;

        // Update cameras
        updateCamera(state.cam1, dt);
        updateCamera(state.cam2, dt);

        // Update rocket
        updateRocket(state.rcktCtrl, dt);

        // Update point lights
        updatePointLights(state.rcktCtrl.model2worldRocket,
                          rocketMesh,
                          pointLights);
        updatePointLightUBO(pointLightUBO, pointLights);


        // Update patricle system
        state.rcktCtrl.particleTimer += dt;  // Accumulate time

        

        // Update particles
        if (state.rcktCtrl.isMoving)
            updateParticles(dt, state.rcktCtrl.particles);

        // Prepare once for entire frame
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // If not split-screen, we do a single render:
        if (!state.isSplitScreen)
        {
            // Build a single projection
            Mat44f proj = make_perspective_projection(
                60.f * std::numbers::pi_v<float> / 180.f,
                float(w)/ float(h),
                0.1f, 100.f
            );

            // We'll use camera #1 and its mode for single-view
            Mat44f view = compute_view_matrix_for_camera(
                              state.cam1, 
                              state.cameraMode1, 
                              state
                          );

            // Render
            renderScene(
                state,
                view, proj,
                langersoVao, langersoMesh, langersoTextureId, langersoVertexCount,
                rocketVao,   rocketMesh,                   rocketVertexCount,
                launchpadVao, launchpadMesh,               launchpadVertexCount,
                particleTextureId
            );
        }
        else
        {
            //  -------- Split Screen (Horizontal) --------
            // We will do the bottom half first with camera1,
            // then the top half with camera2

            // ============= BOTTOM HALF =============
            glViewport(0,0, w, h/2);

            Mat44f proj1 = make_perspective_projection(
                60.f * std::numbers::pi_v<float> / 180.f,
                float(w)/ float(h/2),      // aspect 
                0.1f, 100.f
            );
            Mat44f view1 = compute_view_matrix_for_camera(
                               state.cam1,
                               state.cameraMode1,
                               state
                           );

            renderScene(
                state,
                view1, proj1,
                langersoVao, langersoMesh, langersoTextureId, langersoVertexCount,
                rocketVao,   rocketMesh,                   rocketVertexCount,
                launchpadVao, launchpadMesh,               launchpadVertexCount,
                particleTextureId
            );

            // ============= TOP HALF =============
            glViewport(0, h/2, w, h/2);

            Mat44f proj2 = make_perspective_projection(
                60.f * std::numbers::pi_v<float> / 180.f,
                float(w)/ float(h/2),      // aspect
                0.1f, 100.f
            );
            Mat44f view2 = compute_view_matrix_for_camera(
                               state.cam2,
                               state.cameraMode2,
                               state
                           );

            renderScene(
                state,
                view2, proj2,
                langersoVao, langersoMesh, langersoTextureId, langersoVertexCount,
                rocketVao,   rocketMesh,                   rocketVertexCount,
                launchpadVao, launchpadMesh,               launchpadVertexCount,
                particleTextureId
            );
        }

        // Swap buffers
        glfwSwapBuffers(window);
    }

    // Cleanup
    state.prog = nullptr;    
    return 0;
}
catch(std::exception const& eErr)
{
    std::fprintf(stderr, "Top-level Exception (%s):\n", typeid(eErr).name());
    std::fprintf(stderr, "%s\n", eErr.what());
    std::fprintf(stderr, "Bye.\n");
    return 1;
}


// ----------------- Implementation of the "renderScene" Function -----------------
namespace
{

    // This function draws all objects (Langerso, Rocket, Launchpads, etc.)
    // for a single camera's "view" and "projection".
    void renderScene(State_& state,
                     const Mat44f& view,
                     const Mat44f& projection,
                     GLuint langersoVao, const SimpleMeshData& langersoMesh, GLuint langersoTextureId, size_t langersoCount,
                     GLuint rocketVao,   const SimpleMeshData& rocketMesh, size_t rocketCount,
                     GLuint launchpadVao,const SimpleMeshData& launchpadMesh, size_t launchpadCount,
                     GLuint particleTextureId
    )
    {
        // Use the shader program
        glUseProgram(state.prog->programId());

        // Common light direction & color
        Vec3f lightDir = normalize(Vec3f{0.f, 1.f, -1.f});
        glUniform3fv(2,1, &lightDir.x);
        glUniform3f(3, 0.678f, 0.847f, 0.902f);
        glUniform3f(4, 0.05f, 0.05f, 0.05f);

        // 1) -------------- Langerso --------------
        {
            Mat44f model2world = kIdentity44f;
            Mat33f normalMatrix = mat44_to_mat33(transpose(invert(model2world)));
            Mat44f mvp = projection * view * model2world;

            glUniformMatrix4fv(0,1,GL_TRUE, mvp.v);
            glUniformMatrix3fv(1,1,GL_TRUE, normalMatrix.v);

            // Texture:
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, langersoTextureId);
            glUniform1i(5, langersoMesh.isTextureSupplied);  // location=5

            glUniform2f(6, langersoMesh.mins.x,  langersoMesh.mins.y);   // location=6
            glUniform2f(7, langersoMesh.diffs.x, langersoMesh.diffs.y); // location=7

            glBindVertexArray(langersoVao);
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)langersoCount);

            glBindTexture(GL_TEXTURE_2D, 0);
        }

        // 2) -------------- Rocket --------------
        {
            Mat44f model2world = state.rcktCtrl.model2worldRocket;
            Mat33f normalMatrix = mat44_to_mat33(transpose(invert(model2world)));
            Mat44f mvp          = projection * view * model2world;

            glUniformMatrix4fv(0,1,GL_TRUE, mvp.v);
            glUniformMatrix3fv(1,1,GL_TRUE, normalMatrix.v);
            glUniform1i(5, rocketMesh.isTextureSupplied);

            glBindVertexArray(rocketVao);
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)rocketCount);
        }

        // 3) -------------- Launchpad #1 --------------
        {
            // For convenience, let's say the loaded mesh is already pre-transformed
            Mat44f model2world = kIdentity44f;
            Mat33f normalMatrix = mat44_to_mat33(transpose(invert(model2world)));
            Mat44f mvp          = projection * view * model2world;

            glUniformMatrix4fv(0,1,GL_TRUE, mvp.v);
            glUniformMatrix3fv(1,1,GL_TRUE, normalMatrix.v);
            glUniform1i(5, launchpadMesh.isTextureSupplied);

            glBindVertexArray(launchpadVao);
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)launchpadCount);
        }

        // 4) -------------- Launchpad #2 --------------
        {
            // Another instance of the same mesh, but with an offset transform
            Mat44f model2world = make_translation({3.f,0.f,-5.f});
            Mat33f normalMatrix = mat44_to_mat33(transpose(invert(model2world)));
            Mat44f mvp          = projection * view * model2world;

            glUniformMatrix4fv(0,1,GL_TRUE, mvp.v);
            glUniformMatrix3fv(1,1,GL_TRUE, normalMatrix.v);
            glUniform1i(5, launchpadMesh.isTextureSupplied);

            glBindVertexArray(launchpadVao);
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)launchpadCount);
        }

        // 5) -------------- Particle Exhaust --------------
        {           
            renderParticles(state.rcktCtrl.particles, state.particleShader->programId(), particleTextureId, projection * view);
        }
    }

} // end namespace


