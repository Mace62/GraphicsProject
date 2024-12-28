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
#include <chrono>
#include <ctime>

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

#define M_PI 3.14159265358979323846

#if defined(_WIN32)
extern "C" {
    __declspec(dllexport) unsigned long NvOptimusEnablement = 1;
    __declspec(dllexport) unsigned long AmdPowerXpressRequestHighPerformance = 1;
}
#endif

// ------------- Paths -------------
const std::string DIR_PATH = std::filesystem::current_path().string();

const std::string LANGERSO_OBJ_ASSET_PATH = DIR_PATH + "/assets/cw2/langerso.obj";
const std::string LANGERSO_TEXTURE_ASSET_PATH = DIR_PATH + "/assets/cw2/L3211E-4k.jpg";
const std::string LAUNCHPAD_OBJ_ASSET_PATH = DIR_PATH + "/assets/cw2/landingpad.obj";
const std::string PARTICLE_TEXTURE_PATH = DIR_PATH + "/assets/cw2/particle_smoke.png";

// We assume your geometry shaders are named:
//   "assets/cw2/default.vert" and "assets/cw2/default.frag"
// and your new separate particle shaders are named:
//   "assets/cw2/particle.vert" and "assets/cw2/particle.frag"


// ------------- Some Constants -------------
static constexpr int MAX_POINT_LIGHTS = 3;
static constexpr size_t kMaxParticles = 500;
static constexpr Vec3f rocketStartPos = { 0.0f, 0.0f, 0.0f };

namespace
{
    // Basic camera mode
    enum class CameraMode {
        FREE = 0,
        CHASE,
        GROUND
    };

    // Basic window/camera config
    constexpr char const* kWindowTitle = "COMP3811 - CW2 (Separate Particle Shaders)";
    constexpr float kMovementPerSecond_ = 5.f;
    constexpr float kMouseSensitivity_ = 0.01f;
    constexpr float rocketAcceleration_ = 0.1f;

    using Clock = std::chrono::high_resolution_clock;
    using Secondsf = std::chrono::duration<float>;

    // -------------- Structures for your state --------------
    struct State_
    {
        // Two separate programs: one for geometry, one for particles
        ShaderProgram* geomProg = nullptr;
        ShaderProgram* particleProg = nullptr;

        bool isSplitScreen = false;

        // Camera modes
        CameraMode cameraMode1 = CameraMode::FREE;
        CameraMode cameraMode2 = CameraMode::CHASE;

        struct CamCtrl_
        {
            float FAST_SPEED_MULT = 2.f;
            float SLOW_SPEED_MULT = 0.1f;
            float NORMAL_SPEED_MULT = 0.5f;

            bool movingForward = false;
            bool movingBack = false;
            bool movingLeft = false;
            bool movingRight = false;
            bool movingUp = false;
            bool movingDown = false;

            Vec4f position = { 0.f,5.f,0.f,1.f };
            Vec4f forward = { 0.f,0.f,-1.f,0.f };
            Vec4f right = { 1.f,0.f,0.f,0.f };
            Vec4f up = { 0.f,1.f,0.f,0.f };

            bool cameraActive = false;
            bool actionZoomIn = false, actionZoomOut = false;

            float phi = 0.f, theta = 0.f;
            float radius = 10.f;
            float speed_multiplier = NORMAL_SPEED_MULT;

            float lastX = 0.f, lastY = 0.f, lastTheta = 0.f;
        };

        CamCtrl_ cam1;
        CamCtrl_ cam2;

        // For lighting
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

        // Rocket
        struct rcktCtrl_ {
            Vec3f position = rocketStartPos;
            Vec3f velocity = { 0.f,0.f,0.f };
            Mat44f model2worldRocket = kIdentity44f;
            float acceleration = rocketAcceleration_;
            float time = 0.f;
            bool isMoving = false;
            float pitch = 0.f, yaw = 0.f;

            void reset() {
                model2worldRocket = kIdentity44f;
                position = rocketStartPos;
                velocity = { 0.f,0.f,0.f };
                acceleration = rocketAcceleration_;
                time = 0.f;
                isMoving = false;
                pitch = 0.f;
            }
        } rcktCtrl;

        float chaseDistance = 1.0f;
        Vec3f groundCameraPos = { -5.f,1.0f,0.f };
    };

    // -------------- Particles --------------
    struct Particle {
        Vec3f position;
        Vec3f velocity;
        float life;
        bool active;
    };
    static Particle gParticles[kMaxParticles];
    static GLuint gParticleVAO = 0, gParticleVBO = 0;
    static GLuint gParticleTex = 0; // alpha texture

    // -------------- Function prototypes --------------
    void initParticleSystem();
    void spawnParticle(const Vec3f& pos, const Vec3f& dir);
    void updateParticleSystem(float dt);
    void renderParticles(GLuint progId, const Mat44f& view, const Mat44f& proj);

    // For user input callbacks
    void glfw_callback_error_(int, char const*);
    void glfw_callback_key_(GLFWwindow*, int, int, int, int);
    void glfw_callback_motion_(GLFWwindow*, double, double);
    void mouse_button_callback(GLFWwindow*, int, int, int);

    // For rocket & camera
    void updateCamera(State_::CamCtrl_& camera, float dt);
    void updateRocket(State_::rcktCtrl_& rocket, float dt);

    // For building the camera matrix
    Mat44f compute_view_matrix_for_camera(const State_::CamCtrl_& cam, CameraMode mode, const State_& st);

    // For lights
    GLuint setPointLights(State_::PointLight pointLights[MAX_POINT_LIGHTS], SimpleMeshData rocketPos);
    void updatePointLights(Mat44f rocketPosition, SimpleMeshData rocketData, State_::PointLight pointLights[MAX_POINT_LIGHTS]);
    void updatePointLightUBO(GLuint pointLightUBO, State_::PointLight pointLights[MAX_POINT_LIGHTS]);

    // For your geometry rendering
    void renderScene(const State_& st,
        const Mat44f& view,
        const Mat44f& proj,
        GLuint langersoVao, const SimpleMeshData& langersoMesh, GLuint langersoTex, size_t langersoCount,
        GLuint rocketVao, const SimpleMeshData& rocketMesh, size_t rocketCount,
        GLuint launchpadVao, const SimpleMeshData& launchpadMesh, size_t launchpadCount);

    // RAII
    struct GLFWCleanupHelper {
        ~GLFWCleanupHelper() { glfwTerminate(); }
    };
    struct GLFWWindowDeleter {
        ~GLFWWindowDeleter() { if (window) glfwDestroyWindow(window); }
        GLFWwindow* window;
    };

} // end anonymous namespace

//-------------------------------------------
// main
//-------------------------------------------
int main() try
{
    if (GLFW_TRUE != glfwInit()) {
        char const* msg = nullptr;
        int e = glfwGetError(&msg);
        throw Error("glfwInit() failed with '%s' (%d)", msg, e);
    }
    GLFWCleanupHelper cleanupGLFW;

    glfwSetErrorCallback(&glfw_callback_error_);

    // Create window
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#   if !defined(NDEBUG)
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#   endif

    GLFWwindow* window = glfwCreateWindow(1280, 720, kWindowTitle, nullptr, nullptr);
    if (!window) {
        char const* msg = nullptr;
        int e = glfwGetError(&msg);
        throw Error("glfwCreateWindow failed with '%s' (%d)", msg, e);
    }
    GLFWWindowDeleter windowDeleter{ window };

    // set user pointer
    State_ state{};
    glfwSetWindowUserPointer(window, &state);

    glfwSetKeyCallback(window, &glfw_callback_key_);
    glfwSetCursorPosCallback(window, &glfw_callback_motion_);
    glfwSetMouseButtonCallback(window, &mouse_button_callback);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw Error("Failed to init GLAD!");
    }

    // Debug info
    std::printf("RENDERER %s\n", glGetString(GL_RENDERER));
    std::printf("VENDOR %s\n", glGetString(GL_VENDOR));
    std::printf("VERSION %s\n", glGetString(GL_VERSION));
    std::printf("SHADING_LANGUAGE_VERSION %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

#   if !defined(NDEBUG)
    setup_gl_debug_output();
#   endif

    glClearColor(0.2f, 0.2f, 0.2f, 1.f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FRAMEBUFFER_SRGB);

    // Framebuffer size
    int iwidth, iheight;
    glfwGetFramebufferSize(window, &iwidth, &iheight);
    glViewport(0, 0, iwidth, iheight);

    // 1) Build geometry program from your "default" shaders
    ShaderProgram geomProg({
        { GL_VERTEX_SHADER,   "assets/cw2/default.vert" },
        { GL_FRAGMENT_SHADER, "assets/cw2/default.frag" }
        });
    state.geomProg = &geomProg;

    // 2) Build a *separate* program for particles
    ShaderProgram particleProg({
        { GL_VERTEX_SHADER,   "assets/cw2/particle.vert" },
        { GL_FRAGMENT_SHADER, "assets/cw2/particle.frag" }
        });
    state.particleProg = &particleProg;

    // 3) Load your geometry/meshes (example)
    auto langersoMesh = load_wavefront_obj(LANGERSO_OBJ_ASSET_PATH.c_str(), true);
    GLuint langersoVao = create_vao(langersoMesh);
    GLuint langersoTex = load_texture_2d(LANGERSO_TEXTURE_ASSET_PATH.c_str());
    size_t langersoCount = langersoMesh.positions.size();

    auto launchpadMesh = load_wavefront_obj(
        LAUNCHPAD_OBJ_ASSET_PATH.c_str(),
        false,
        make_translation({ 2.f,0.005f,-2.f }) * make_scaling(0.5f, 0.5f, 0.5f)
    );
    GLuint launchpadVao = create_vao(launchpadMesh);
    size_t launchpadCount = launchpadMesh.positions.size();

    auto rocketMesh = create_spaceship(
        32,
        { 0.2f,0.2f,0.2f }, { 0.8f,0.2f,0.2f },
        make_translation({ 2.f,0.15f,-2.f }) * make_scaling(0.05f, 0.05f, 0.05f),
        false
    );
    GLuint rocketVao = create_vao(rocketMesh);
    size_t rocketCount = rocketMesh.positions.size();

    // 4) Lights
    State_::PointLight pointLights[MAX_POINT_LIGHTS];
    GLuint pointLightUBO = setPointLights(pointLights, rocketMesh);

    // 5) Particle system init
    initParticleSystem();

    // 6) main loop
    auto last = std::chrono::high_resolution_clock::now();
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        if (w <= 0 || h <= 0) {
            do {
                glfwWaitEvents();
                glfwGetFramebufferSize(window, &w, &h);
            } while (w <= 0 || h <= 0);
        }
        glViewport(0, 0, w, h);

        auto now = Clock::now();
        float dt = std::chrono::duration_cast<Secondsf>(now - last).count();
        last = now;

        // update rocket/cameras
        updateCamera(state.cam1, dt);
        updateCamera(state.cam2, dt);
        updateRocket(state.rcktCtrl, dt);

        // spawn particles if rocket is moving
        if (state.rcktCtrl.isMoving) {
            Vec3f rocketForwardWS = { 0.f,1.f,0.f }; // or from orientation
            Vec3f exhaustPos = state.rcktCtrl.position - 0.2f * rocketForwardWS;
            for (int i = 0; i < 10; i++) {
                spawnParticle(exhaustPos, rocketForwardWS);
            }
        }

        updateParticleSystem(dt);

        // update lights
        updatePointLights(state.rcktCtrl.model2worldRocket, rocketMesh, pointLights);
        updatePointLightUBO(pointLightUBO, pointLights);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Decide how to do cameras
        if (!state.isSplitScreen) {
            Mat44f proj = make_perspective_projection(
                60.f * std::numbers::pi_v<float> / 180.f,
                float(w) / float(h),
                0.1f, 100.f
            );
            Mat44f view = compute_view_matrix_for_camera(state.cam1, state.cameraMode1, state);

            // Geometry pass with geomProg
            renderScene(state, view, proj,
                langersoVao, langersoMesh, langersoTex, langersoCount,
                rocketVao, rocketMesh, rocketCount,
                launchpadVao, launchpadMesh, launchpadCount);

            // Particle pass with particleProg
            renderParticles(particleProg.programId(), view, proj);
        }
        else {
            // Split screen
            // bottom half
            glViewport(0, 0, w, h / 2);

            Mat44f proj1 = make_perspective_projection(
                60.f * std::numbers::pi_v<float> / 180.f,
                float(w) / float(h / 2),
                0.1f, 100.f
            );
            Mat44f view1 = compute_view_matrix_for_camera(state.cam1, state.cameraMode1, state);

            renderScene(state, view1, proj1,
                langersoVao, langersoMesh, langersoTex, langersoCount,
                rocketVao, rocketMesh, rocketCount,
                launchpadVao, launchpadMesh, launchpadCount);

            renderParticles(particleProg.programId(), view1, proj1);

            // top half
            glViewport(0, h / 2, w, h / 2);

            Mat44f proj2 = make_perspective_projection(
                60.f * std::numbers::pi_v<float> / 180.f,
                float(w) / float(h / 2),
                0.1f, 100.f
            );
            Mat44f view2 = compute_view_matrix_for_camera(state.cam2, state.cameraMode2, state);

            renderScene(state, view2, proj2,
                langersoVao, langersoMesh, langersoTex, langersoCount,
                rocketVao, rocketMesh, rocketCount,
                launchpadVao, launchpadMesh, launchpadCount);

            renderParticles(particleProg.programId(), view2, proj2);
        }

        glfwSwapBuffers(window);
    }

    state.geomProg = nullptr;
    state.particleProg = nullptr;
    return 0;
}
catch (std::exception const& e) {
    std::fprintf(stderr, "Top-level Error: %s\n", e.what());
    return 1;
}


// ---------------------------------------------------
// Implementation of the rest
// ---------------------------------------------------
namespace
{
    // ******** Particle system code ********
    void initParticleSystem()
    {
        std::srand(unsigned(std::time(nullptr)));

        glGenVertexArrays(1, &gParticleVAO);
        glBindVertexArray(gParticleVAO);

        glGenBuffers(1, &gParticleVBO);
        glBindBuffer(GL_ARRAY_BUFFER, gParticleVBO);
        glBufferData(GL_ARRAY_BUFFER, kMaxParticles * sizeof(Vec3f),
            nullptr, GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vec3f), (void*)0);

        glBindVertexArray(0);

        gParticleTex = load_texture_2d(PARTICLE_TEXTURE_PATH.c_str());

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        glEnable(GL_PROGRAM_POINT_SIZE);

        for (size_t i = 0; i < kMaxParticles; i++) {
            gParticles[i].active = false;
        }
    }

    void spawnParticle(const Vec3f& pos, const Vec3f& dir)
    {
        for (size_t i = 0; i < kMaxParticles; i++) {
            if (!gParticles[i].active) {
                gParticles[i].active = true;
                gParticles[i].position = pos;
                float spread = 0.2f;
                Vec3f rnd = dir + Vec3f(float(std::rand()) / RAND_MAX * 2.f * spread - spread,
                    float(std::rand()) / RAND_MAX * 2.f * spread - spread,
                    float(std::rand()) / RAND_MAX * 2.f * spread - spread);
                rnd = normalize(rnd);
                gParticles[i].velocity = rnd * 3.f;
                gParticles[i].life = 2.f;
                return;
            }
        }
    }

    void updateParticleSystem(float dt)
    {
        for (size_t i = 0; i < kMaxParticles; i++) {
            if (!gParticles[i].active) continue;
            gParticles[i].position += gParticles[i].velocity * dt;
            gParticles[i].life -= dt;
            if (gParticles[i].life <= 0.f) {
                gParticles[i].active = false;
            }
        }
    }

    void renderParticles(GLuint progId, const Mat44f& view, const Mat44f& proj)
    {
        glUseProgram(progId);

        GLint locView = glGetUniformLocation(progId, "uView");
        GLint locProj = glGetUniformLocation(progId, "uProj");
        GLint locTex = glGetUniformLocation(progId, "uTex");

        if (locView >= 0) glUniformMatrix4fv(locView, 1, GL_TRUE, view.v);
        if (locProj >= 0) glUniformMatrix4fv(locProj, 1, GL_TRUE, proj.v);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gParticleTex);
        if (locTex >= 0) glUniform1i(locTex, 0);

        static std::vector<Vec3f> positions(kMaxParticles);
        size_t count = 0;
        for (size_t i = 0; i < kMaxParticles; i++) {
            if (!gParticles[i].active) continue;
            positions[count++] = gParticles[i].position;
        }

        glBindBuffer(GL_ARRAY_BUFFER, gParticleVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(Vec3f), positions.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindVertexArray(gParticleVAO);
        glDrawArrays(GL_POINTS, 0, (GLsizei)count);
    }

    // ******* Rocket & camera code *******
    void updateCamera(State_::CamCtrl_& c, float dt)
    {
        float speed = kMovementPerSecond_ * dt * c.speed_multiplier;
        c.forward = Vec4f{
            std::sin(c.phi) * std::cos(c.theta),
            -std::sin(c.theta),
            -std::cos(c.phi) * std::cos(c.theta),
            0.f
        };
        c.right = normalize(cross(c.forward, Vec4f{ 0.f,1.f,0.f,0.f }));
        c.up = cross(c.right, c.forward);

        Vec4f movement(0, 0, 0, 0);
        if (c.movingForward) movement += c.forward * speed;
        if (c.movingBack)    movement -= c.forward * speed;
        if (c.movingRight)   movement += c.right * speed;
        if (c.movingLeft)    movement -= c.right * speed;
        if (c.movingUp)      movement += c.up * speed;
        if (c.movingDown)    movement -= c.up * speed;
        c.position += movement;
    }

    void updateRocket(State_::rcktCtrl_& rocket, float dt)
    {
        if (!rocket.isMoving) return;

        rocket.time += dt;
        Vec3f prev = rocket.position;

        // Some logic
        if (rocket.time <= 5.f) {
            rocket.velocity.y += 2.f * dt;
        }
        else {
            rocket.velocity += normalize(Vec3f(3.f, 1.f, -3.5f)) * dt * rocket.acceleration;
        }

        rocket.position += rocket.velocity * dt;

        Vec3f dir = rocket.position - prev;
        if (length(dir) > 1e-3f) {
            dir = normalize(dir);
        }

        float pitch = atan2(dir.z, sqrt(dir.x * dir.x + dir.y * dir.y));
        float yaw = atan2(dir.x, dir.y);

        Mat44f rotPitch = make_rotation_x(pitch);
        Mat44f rotYaw = make_rotation_z(-yaw);
        Mat44f rotation = rotYaw * rotPitch;
        Mat44f transl = make_translation(rocket.position);
        rocket.model2worldRocket = transl * rotation;
    }

    // ******* camera logic *******
    Mat44f compute_view_matrix_for_camera(const State_::CamCtrl_& cam, CameraMode mode, const State_& st)
    {
        switch (mode)
        {
        case CameraMode::FREE:
            return make_look_at(
                cam.position,
                cam.position + cam.forward,
                cam.up
            );
        case CameraMode::CHASE:
        {
            auto& rocketPos = st.rcktCtrl.position;
            Vec3f chasePos = rocketPos - Vec3f{ 0.f,0.f,1.f }*st.chaseDistance + Vec3f{ 0.f,1.f,0.f };
            return make_look_at(
                Vec4f{ chasePos.x,chasePos.y,chasePos.z,1.f },
                Vec4f{ rocketPos.x + 1.47f,rocketPos.y,rocketPos.z - 1.2f,1.f },
                Vec4f{ 0.f,1.f,0.f,0.f }
            );
        }
        case CameraMode::GROUND:
        {
            auto& rp = st.rcktCtrl.position;
            Vec4f rocketPos4 = { rp.x + 1.47f,rp.y,rp.z - 1.2f,1.f };
            Vec4f groundPos4 = { st.groundCameraPos.x, st.groundCameraPos.y, st.groundCameraPos.z,1.f };
            return make_look_at(groundPos4, rocketPos4, Vec4f{ 0.f,1.f,0.f,0.f });
        }
        }
        return kIdentity44f;
    }

    // ******* Lights *******
    GLuint setPointLights(State_::PointLight pls[MAX_POINT_LIGHTS], SimpleMeshData rocketPos)
    {
        pls[0].position = rocketPos.pointLightPos[0];
        pls[0].color = Vec3f{ 1.f,0.f,0.f };
        pls[0].radius = 1.f;
        pls[0].normals = rocketPos.pointLightNorms[0];
        // likewise for [1] & [2], etc.

        GLuint ubo;
        glGenBuffers(1, &ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, ubo);

        State_::PointLightBlock block;
        for (int i = 0; i < MAX_POINT_LIGHTS; i++) {
            block.lights[i] = pls[i];
        }
        glBufferData(GL_UNIFORM_BUFFER, sizeof(block), &block, GL_STATIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, 1, ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        return ubo;
    }

    void updatePointLights(Mat44f rocketPos, SimpleMeshData rocketData, State_::PointLight pls[MAX_POINT_LIGHTS])
    {
        Mat33f N = mat44_to_mat33(transpose(invert(rocketPos)));
        for (int i = 0; i < MAX_POINT_LIGHTS; i++) {
            pls[i].position = rocketData.pointLightPos[i];
            // transform if you want
            pls[i].normals = normalize(N * rocketData.pointLightNorms[i]);
        }
    }

    void updatePointLightUBO(GLuint ubo, State_::PointLight pls[MAX_POINT_LIGHTS])
    {
        State_::PointLightBlock block;
        for (int i = 0; i < MAX_POINT_LIGHTS; i++) {
            block.lights[i] = pls[i];
        }
        glBindBuffer(GL_UNIFORM_BUFFER, ubo);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(block), &block);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    // ******* renderScene *******
    void renderScene(const State_& st,
        const Mat44f& view,
        const Mat44f& proj,
        GLuint langersoVao, const SimpleMeshData& langersoMesh,
        GLuint langersoTex, size_t langersoCount,
        GLuint rocketVao, const SimpleMeshData& rocketMesh, size_t rocketCount,
        GLuint launchpadVao, const SimpleMeshData& launchpadMesh, size_t launchpadCount)
    {
        // Use geometry program
        glUseProgram(st.geomProg->programId());

        // set common uniforms
        // e.g. light direction, etc.
        Vec3f lightDir = normalize(Vec3f{ 0.f,1.f,-1.f });
        glUniform3fv(2, 1, &lightDir.x);
        glUniform3f(3, 0.678f, 0.847f, 0.902f);
        glUniform3f(4, 0.05f, 0.05f, 0.05f);

        // For each object (langerso, rocket, launchpad),
        // we build the model->world->view->proj matrix
        // and send it to your default shaders

        // 1) Langerso
        {
            Mat44f model2world = kIdentity44f;
            Mat33f normalMatrix = mat44_to_mat33(transpose(invert(model2world)));
            Mat44f mvp = proj * view * model2world;

            glUniformMatrix4fv(0, 1, GL_TRUE, mvp.v);
            glUniformMatrix3fv(1, 1, GL_TRUE, normalMatrix.v);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, langersoTex);
            glUniform1i(5, langersoMesh.isTextureSupplied);

            glUniform2f(6, langersoMesh.mins.x, langersoMesh.mins.y);
            glUniform2f(7, langersoMesh.diffs.x, langersoMesh.diffs.y);

            glBindVertexArray(langersoVao);
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)langersoCount);

            glBindTexture(GL_TEXTURE_2D, 0);
        }

        // 2) Rocket
        {
            // e.g. rocket transform from st.rcktCtrl
            Mat44f model2world = st.rcktCtrl.model2worldRocket;
            Mat33f normalMatrix = mat44_to_mat33(transpose(invert(model2world)));
            Mat44f mvp = proj * view * model2world;

            glUniformMatrix4fv(0, 1, GL_TRUE, mvp.v);
            glUniformMatrix3fv(1, 1, GL_TRUE, normalMatrix.v);
            glUniform1i(5, rocketMesh.isTextureSupplied);

            glBindVertexArray(rocketVao);
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)rocketCount);
        }

        // 3) Launchpad #1
        {
            Mat44f model2world = kIdentity44f;
            Mat33f normalMatrix = mat44_to_mat33(transpose(invert(model2world)));
            Mat44f mvp = proj * view * model2world;

            glUniformMatrix4fv(0, 1, GL_TRUE, mvp.v);
            glUniformMatrix3fv(1, 1, GL_TRUE, normalMatrix.v);
            glUniform1i(5, launchpadMesh.isTextureSupplied);

            glBindVertexArray(launchpadVao);
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)launchpadCount);
        }

        // 4) Launchpad #2 
        {
            Mat44f model2world = make_translation({ 3.f,0.f,-5.f });
            Mat33f normalMatrix = mat44_to_mat33(transpose(invert(model2world)));
            Mat44f mvp = proj * view * model2world;

            glUniformMatrix4fv(0, 1, GL_TRUE, mvp.v);
            glUniformMatrix3fv(1, 1, GL_TRUE, normalMatrix.v);
            glUniform1i(5, launchpadMesh.isTextureSupplied);

            glBindVertexArray(launchpadVao);
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)launchpadCount);
        }
    }

    // ********** Callbacks for input (just stubs or partial) **********
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
                state->rcktCtrl.reset();
               /* if (state->prog) {
                    
                    try {
                        state->prog->reload();
                        std::fprintf(stderr, "Shaders reloaded and recompiled.\n");
                    }
                    catch (std::exception const& eErr) {
                        std::fprintf(stderr, "Error when reloading shader:\n");
                        std::fprintf(stderr, "%s\n", eErr.what());
                        std::fprintf(stderr, "Keeping old shader.\n");
                    }
                }*/
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

} // end anonymous namespace
