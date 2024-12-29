// Uncomment to ENABLE performance measurements
#define ENABLE_PERFORMANCE_METRICS

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

// ------------------ Quick Typedefs --------------------
using Clock = std::chrono::high_resolution_clock;
using Secondsf = std::chrono::duration<float>;

#define M_PI 3.14159265358979323846

#if defined(_WIN32)
extern "C"
{
    __declspec(dllexport) unsigned long NvOptimusEnablement = 1;
    __declspec(dllexport) unsigned long AmdPowerXpressRequestHighPerformance = 1; // untested
}
#endif

// ------------------- Assets & Constants --------------------
const std::string DIR_PATH = std::filesystem::current_path().string();

const std::string LANGERSO_OBJ_ASSET_PATH = DIR_PATH + "/assets/cw2/langerso.obj";
const std::string LANGERSO_TEXTURE_ASSET_PATH = DIR_PATH + "/assets/cw2/L3211E-4k.jpg";
const std::string LAUNCHPAD_OBJ_ASSET_PATH = DIR_PATH + "/assets/cw2/landingpad.obj";

static constexpr int   MAX_POINT_LIGHTS = 3;
static constexpr Vec3f rocketStartPos = { 0.0f, 0.0f, 0.0f };

// --------------- Camera Mode ---------------
enum class CameraMode {
    FREE = 0,
    CHASE,
    GROUND
};

// --------------- Window Title & Movement Constants -------------
constexpr char const* kWindowTitle = "COMP3811 - CW2";
constexpr float       kMovementPerSecond_ = 5.f;    // movement units per second
constexpr float       kMouseSensitivity_ = 0.01f;  // radians per pixel
constexpr float       rocketAcceleration_ = 0.1f;

// --------------- Program State ---------------
struct State_
{
    ShaderProgram* prog = nullptr;

    // Split-screen toggle
    bool isSplitScreen = false;

    // Two cameras
    CameraMode cameraMode1 = CameraMode::FREE;
    CameraMode cameraMode2 = CameraMode::CHASE;

    // Camera control
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

        // Orientation
        Vec4f position = { 0.0f, 5.0f, 0.0f, 1.0f };
        Vec4f forward = { 0.0f, 0.0f, -1.0f, 0.0f };
        Vec4f right = { 1.0f, 0.0f,  0.0f, 0.0f };
        Vec4f up = { 0.0f, 1.0f,  0.0f, 0.0f };

        bool  cameraActive = false;
        bool  actionZoomIn = false;
        bool  actionZoomOut = false;

        float phi = 0.0f; // yaw
        float theta = 0.0f; // pitch
        float radius = 10.f;
        float speed_multiplier = NORMAL_SPEED_MULT;

        float lastX = 0.f, lastY = 0.f, lastTheta = 0.f;
    };

    CamCtrl_ cam1;
    CamCtrl_ cam2;

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
    struct rcktCtrl_
    {
        Vec3f position = rocketStartPos;
        Vec3f velocity = { 0.f, 0.f, 0.f };
        Mat44f model2worldRocket = kIdentity44f;
        float acceleration = rocketAcceleration_;
        float time = 0.f;
        bool  isMoving = false;
        float pitch = 0.f;
        float yaw = 0.f;

        void reset() {
            model2worldRocket = kIdentity44f;
            position = rocketStartPos;
            velocity = { 0.0f, 0.0f, 0.0f };
            acceleration = rocketAcceleration_;
            time = 0.f;
            isMoving = false;
            pitch = 0.f;
        }
    } rcktCtrl;

    float chaseDistance = 1.0f;
    Vec3f groundCameraPos = { -5.f, 1.0f, 0.f };
};

// ------------------ Forward Declarations ------------------
static void glfw_callback_error_(int, char const*);
static void glfw_callback_key_(GLFWwindow*, int, int, int, int);
static void glfw_callback_motion_(GLFWwindow*, double, double);
static void mouse_button_callback(GLFWwindow*, int, int, int);

static void updateCamera(State_::CamCtrl_& camera, float dt);
static void updateRocket(State_::rcktCtrl_& rocket, float dt);

static Mat44f compute_view_matrix_for_camera(const State_::CamCtrl_& camCtrl,
    CameraMode mode,
    const State_& state);

static GLuint setPointLights(State_::PointLight pointLights[MAX_POINT_LIGHTS],
    SimpleMeshData rocketPos);

static void updatePointLights(Mat44f rocketPosition,
    SimpleMeshData rocketData,
    State_::PointLight pointLights[MAX_POINT_LIGHTS]);

static void updatePointLightUBO(GLuint pointLightUBO,
    State_::PointLight pointLights[MAX_POINT_LIGHTS]);

static void renderScene(const State_& state,
    const Mat44f& view,
    const Mat44f& projection,
    GLuint langersoVao, const SimpleMeshData& langersoMesh, GLuint langersoTextureId, size_t langersoCount,
    GLuint rocketVao, const SimpleMeshData& rocketMesh, size_t rocketCount,
    GLuint launchpadVao, const SimpleMeshData& launchpadMesh, size_t launchpadCount);

// RAII-like helpers
struct GLFWCleanupHelper
{
    ~GLFWCleanupHelper() { glfwTerminate(); }
};
struct GLFWWindowDeleter
{
    ~GLFWWindowDeleter() { if (window) glfwDestroyWindow(window); }
    GLFWwindow* window;
};


// --------------- Perf Measurement ---------------
#ifdef ENABLE_PERFORMANCE_METRICS
static constexpr int MAX_FRAMES_IN_FLIGHT = 3;

// Queries for total frame time
static GLuint g_timestampFrameStart[MAX_FRAMES_IN_FLIGHT];
static GLuint g_timestampFrameEnd[MAX_FRAMES_IN_FLIGHT];

// Queries for terrain, launchpads, spaceship
static GLuint g_timestampTerrainStart[MAX_FRAMES_IN_FLIGHT];
static GLuint g_timestampTerrainEnd[MAX_FRAMES_IN_FLIGHT];
static GLuint g_timestampLaunchpadsStart[MAX_FRAMES_IN_FLIGHT];
static GLuint g_timestampLaunchpadsEnd[MAX_FRAMES_IN_FLIGHT];
static GLuint g_timestampSpaceshipStart[MAX_FRAMES_IN_FLIGHT];
static GLuint g_timestampSpaceshipEnd[MAX_FRAMES_IN_FLIGHT];

// Extra queries for split-screen mode (View A & View B)
static GLuint g_timestampViewAStart[MAX_FRAMES_IN_FLIGHT];
static GLuint g_timestampViewAEnd[MAX_FRAMES_IN_FLIGHT];
static GLuint g_timestampViewBStart[MAX_FRAMES_IN_FLIGHT];
static GLuint g_timestampViewBEnd[MAX_FRAMES_IN_FLIGHT];

// CPU timing arrays
static double g_cpuRenderTimes[MAX_FRAMES_IN_FLIGHT] = {};
static double g_cpuFrameTimes[MAX_FRAMES_IN_FLIGHT] = {};

static int g_currentFrameIndex = 0;
static int g_totalFrameCount = 0;

// Helper to retrieve queries from an older frame
static void retrieveQueries(int frameIndex)
{
    GLuint64 frameStart = 0, frameEnd = 0;
    glGetQueryObjectui64v(g_timestampFrameStart[frameIndex], GL_QUERY_RESULT, &frameStart);
    glGetQueryObjectui64v(g_timestampFrameEnd[frameIndex], GL_QUERY_RESULT, &frameEnd);

    double fullFrameMs = double(frameEnd - frameStart) * 1e-6;

    // Terrain
    GLuint64 tStart = 0, tEnd = 0;
    glGetQueryObjectui64v(g_timestampTerrainStart[frameIndex], GL_QUERY_RESULT, &tStart);
    glGetQueryObjectui64v(g_timestampTerrainEnd[frameIndex], GL_QUERY_RESULT, &tEnd);
    double terrainMs = double(tEnd - tStart) * 1e-6;

    // Launchpads
    GLuint64 lStart = 0, lEnd = 0;
    glGetQueryObjectui64v(g_timestampLaunchpadsStart[frameIndex], GL_QUERY_RESULT, &lStart);
    glGetQueryObjectui64v(g_timestampLaunchpadsEnd[frameIndex], GL_QUERY_RESULT, &lEnd);
    double launchpadsMs = double(lEnd - lStart) * 1e-6;

    // Spaceship
    GLuint64 sStart = 0, sEnd = 0;
    glGetQueryObjectui64v(g_timestampSpaceshipStart[frameIndex], GL_QUERY_RESULT, &sStart);
    glGetQueryObjectui64v(g_timestampSpaceshipEnd[frameIndex], GL_QUERY_RESULT, &sEnd);
    double spaceshipMs = double(sEnd - sStart) * 1e-6;

    // Sub-views
    GLuint64 vaStart = 0, vaEnd = 0, vbStart = 0, vbEnd = 0;
    glGetQueryObjectui64v(g_timestampViewAStart[frameIndex], GL_QUERY_RESULT, &vaStart);
    glGetQueryObjectui64v(g_timestampViewAEnd[frameIndex], GL_QUERY_RESULT, &vaEnd);

    glGetQueryObjectui64v(g_timestampViewBStart[frameIndex], GL_QUERY_RESULT, &vbStart);
    glGetQueryObjectui64v(g_timestampViewBEnd[frameIndex], GL_QUERY_RESULT, &vbEnd);

    double viewAMs = double(vaEnd - vaStart) * 1e-6;
    double viewBMs = double(vbEnd - vbStart) * 1e-6;

    // CPU times
    double cpuRenderTime = g_cpuRenderTimes[frameIndex];
    double cpuFrameTime = g_cpuFrameTimes[frameIndex];

    std::cout << "Frame " << frameIndex << " results:\n"
        << "  GPU times (ms):\n"
        << "    Full Frame    = " << fullFrameMs << "\n"
        << "    Terrain       = " << terrainMs << "\n"
        << "    Launchpads    = " << launchpadsMs << "\n"
        << "    Spaceship     = " << spaceshipMs << "\n"
        << "    View A(bottom)= " << viewAMs << "\n"
        << "    View B(top)   = " << viewBMs << "\n"
        << "  CPU times (ms):\n"
        << "    RenderSubmit  = " << cpuRenderTime << "\n"
        << "    FullFrame     = " << cpuFrameTime << "\n\n";
}
#endif // ENABLE_PERFORMANCE_METRICS

int main() try
{
    // Initialize GLFW
    if (GLFW_TRUE != glfwInit())
    {
        char const* msg = nullptr;
        int ecode = glfwGetError(&msg);
        throw Error("glfwInit() failed with '%s' (%d)", msg, ecode);
    }
    GLFWCleanupHelper cleanupHelper;

    glfwSetErrorCallback(&glfw_callback_error_);

#   if !defined(__APPLE__)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#   else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
#   endif
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#   if !defined(NDEBUG)
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#   endif

    // Create window
    GLFWwindow* window = glfwCreateWindow(1280, 720,
        kWindowTitle,
        nullptr, nullptr);
    if (!window)
    {
        char const* msg = nullptr;
        int ecode = glfwGetError(&msg);
        throw Error("glfwCreateWindow() failed with '%s' (%d)", msg, ecode);
    }
    GLFWWindowDeleter windowDeleter{ window };

    // Attach user pointer
    State_ state{};
    glfwSetWindowUserPointer(window, &state);

    // Input callbacks
    glfwSetKeyCallback(window, &glfw_callback_key_);
    glfwSetCursorPosCallback(window, &glfw_callback_motion_);
    glfwSetMouseButtonCallback(window, &mouse_button_callback);

    // Make context current & vsync
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Glad init
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        throw Error("Failed to initialize GLAD!");

    // Debug prints
    std::printf("RENDERER                   %s\n", glGetString(GL_RENDERER));
    std::printf("VENDOR                     %s\n", glGetString(GL_VENDOR));
    std::printf("VERSION                    %s\n", glGetString(GL_VERSION));
    std::printf("SHADING_LANGUAGE_VERSION   %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

#   if !defined(NDEBUG)
    setup_gl_debug_output();
#   endif

    OGL_CHECKPOINT_ALWAYS();

    // Global GL state
    glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_DEPTH_TEST);

    // Framebuffer size
    int iwidth, iheight;
    glfwGetFramebufferSize(window, &iwidth, &iheight);
    glViewport(0, 0, iwidth, iheight);

    // Load shaders
    ShaderProgram prog({
        {GL_VERTEX_SHADER,   "assets/cw2/default.vert"},
        {GL_FRAGMENT_SHADER, "assets/cw2/default.frag"}
        });
    state.prog = &prog;

    // Load meshes
    auto langersoMesh = load_wavefront_obj(LANGERSO_OBJ_ASSET_PATH.c_str(), true);
    GLuint langersoVao = create_vao(langersoMesh);
    GLuint langersoTextureId = load_texture_2d(LANGERSO_TEXTURE_ASSET_PATH.c_str());
    size_t langersoVertexCount = langersoMesh.positions.size();

    auto launchpadMesh = load_wavefront_obj(
        LAUNCHPAD_OBJ_ASSET_PATH.c_str(),
        false,
        make_translation({ 2.f,0.005f,-2.f }) * make_scaling(0.5f, 0.5f, 0.5f)
    );
    GLuint launchpadVao = create_vao(launchpadMesh);
    size_t launchpadVertexCount = launchpadMesh.positions.size();

    auto rocketMesh = create_spaceship(
        32,
        { 0.2f, 0.2f, 0.2f }, { 0.8f, 0.2f, 0.2f },
        make_translation({ 2.f,0.15f,-2.f }) * make_scaling(0.05f, 0.05f, 0.05f),
        false
    );
    GLuint rocketVao = create_vao(rocketMesh);
    size_t rocketVertexCount = rocketMesh.positions.size();

    // Setup lights
    State_::PointLight pointLights[MAX_POINT_LIGHTS];
    GLuint pointLightUBO = setPointLights(pointLights, rocketMesh);

    OGL_CHECKPOINT_ALWAYS();

    // Timing
    auto last = Clock::now();

#ifdef ENABLE_PERFORMANCE_METRICS
    // Generate all queries
    glGenQueries(MAX_FRAMES_IN_FLIGHT, g_timestampFrameStart);
    glGenQueries(MAX_FRAMES_IN_FLIGHT, g_timestampFrameEnd);

    glGenQueries(MAX_FRAMES_IN_FLIGHT, g_timestampTerrainStart);
    glGenQueries(MAX_FRAMES_IN_FLIGHT, g_timestampTerrainEnd);
    glGenQueries(MAX_FRAMES_IN_FLIGHT, g_timestampLaunchpadsStart);
    glGenQueries(MAX_FRAMES_IN_FLIGHT, g_timestampLaunchpadsEnd);
    glGenQueries(MAX_FRAMES_IN_FLIGHT, g_timestampSpaceshipStart);
    glGenQueries(MAX_FRAMES_IN_FLIGHT, g_timestampSpaceshipEnd);

    glGenQueries(MAX_FRAMES_IN_FLIGHT, g_timestampViewAStart);
    glGenQueries(MAX_FRAMES_IN_FLIGHT, g_timestampViewAEnd);
    glGenQueries(MAX_FRAMES_IN_FLIGHT, g_timestampViewBStart);
    glGenQueries(MAX_FRAMES_IN_FLIGHT, g_timestampViewBEnd);
#endif

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Window resizing
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        if (w <= 0 || h <= 0)
        {
            do {
                glfwWaitEvents();
                glfwGetFramebufferSize(window, &w, &h);
            } while (w <= 0 || h <= 0);
        }
        glViewport(0, 0, w, h);

        // dt
        auto now = Clock::now();
        float dt = std::chrono::duration_cast<Secondsf>(now - last).count();
        last = now;

#ifdef ENABLE_PERFORMANCE_METRICS
        // CPU frame start
        auto cpuFrameStart = Clock::now();

        // GPU frame start
        glQueryCounter(g_timestampFrameStart[g_currentFrameIndex], GL_TIMESTAMP);
#endif

        updateCamera(state.cam1, dt);
        updateCamera(state.cam2, dt);
        updateRocket(state.rcktCtrl, dt);

        updatePointLights(state.rcktCtrl.model2worldRocket, rocketMesh, pointLights);
        updatePointLightUBO(pointLightUBO, pointLights);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#ifdef ENABLE_PERFORMANCE_METRICS
        auto cpuRenderStart = Clock::now();
#endif

        // Queries always used, even if not in split screen
        glQueryCounter(g_timestampViewAStart[g_currentFrameIndex], GL_TIMESTAMP);

        if (!state.isSplitScreen)
        {
            // Single view
            Mat44f proj = make_perspective_projection(
                60.f * std::numbers::pi_v<float> / 180.f,
                float(w) / float(h),
                0.1f, 100.f
            );
            Mat44f view = compute_view_matrix_for_camera(
                state.cam1,
                state.cameraMode1,
                state
            );

            renderScene(
                state,
                view, proj,
                langersoVao, langersoMesh, langersoTextureId, langersoVertexCount,
                rocketVao, rocketMesh, rocketVertexCount,
                launchpadVao, launchpadMesh, launchpadVertexCount
            );
        }
        else
        {
            // Bottom half
            glViewport(0, 0, w, h / 2);

            Mat44f projA = make_perspective_projection(
                60.f * std::numbers::pi_v<float> / 180.f,
                float(w) / float(h / 2),
                0.1f, 100.f
            );
            Mat44f viewA = compute_view_matrix_for_camera(
                state.cam1,
                state.cameraMode1,
                state
            );

            renderScene(
                state,
                viewA, projA,
                langersoVao, langersoMesh, langersoTextureId, langersoVertexCount,
                rocketVao, rocketMesh, rocketVertexCount,
                launchpadVao, launchpadMesh, launchpadVertexCount
            );
            glViewport(0, 0, w, h);
        }

        glQueryCounter(g_timestampViewAEnd[g_currentFrameIndex], GL_TIMESTAMP);


        glQueryCounter(g_timestampViewBStart[g_currentFrameIndex], GL_TIMESTAMP);

        if (state.isSplitScreen)
        {
            // Top Half
            glViewport(0, h / 2, w, h / 2);

            Mat44f projB = make_perspective_projection(
                60.f * std::numbers::pi_v<float> / 180.f,
                float(w) / float(h / 2),
                0.1f, 100.f
            );
            Mat44f viewB = compute_view_matrix_for_camera(
                state.cam2,
                state.cameraMode2,
                state
            );
            renderScene(
                state,
                viewB, projB,
                langersoVao, langersoMesh, langersoTextureId, langersoVertexCount,
                rocketVao, rocketMesh, rocketVertexCount,
                launchpadVao, launchpadMesh, launchpadVertexCount
            );
            glViewport(0, 0, w, h);
        }

        glQueryCounter(g_timestampViewBEnd[g_currentFrameIndex], GL_TIMESTAMP);

        glfwSwapBuffers(window);

#ifdef ENABLE_PERFORMANCE_METRICS
        // GPU frame end
        glQueryCounter(g_timestampFrameEnd[g_currentFrameIndex], GL_TIMESTAMP);

        auto cpuRenderEnd = Clock::now();
        g_cpuRenderTimes[g_currentFrameIndex] =
            std::chrono::duration<double, std::milli>(cpuRenderEnd - cpuRenderStart).count();

        auto cpuFrameEnd = Clock::now();
        g_cpuFrameTimes[g_currentFrameIndex] =
            std::chrono::duration<double, std::milli>(cpuFrameEnd - cpuFrameStart).count();

        g_totalFrameCount++;

        // Retrieve from older frame to avoid stalls
        if (g_totalFrameCount > MAX_FRAMES_IN_FLIGHT)
        {
            int retrieveIndex = (g_currentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
            retrieveQueries(retrieveIndex);
        }

        g_currentFrameIndex = (g_currentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
#endif
    }

    state.prog = nullptr;

#ifdef ENABLE_PERFORMANCE_METRICS
    // Delete queries
    glDeleteQueries(MAX_FRAMES_IN_FLIGHT, g_timestampFrameStart);
    glDeleteQueries(MAX_FRAMES_IN_FLIGHT, g_timestampFrameEnd);

    glDeleteQueries(MAX_FRAMES_IN_FLIGHT, g_timestampTerrainStart);
    glDeleteQueries(MAX_FRAMES_IN_FLIGHT, g_timestampTerrainEnd);

    glDeleteQueries(MAX_FRAMES_IN_FLIGHT, g_timestampLaunchpadsStart);
    glDeleteQueries(MAX_FRAMES_IN_FLIGHT, g_timestampLaunchpadsEnd);

    glDeleteQueries(MAX_FRAMES_IN_FLIGHT, g_timestampSpaceshipStart);
    glDeleteQueries(MAX_FRAMES_IN_FLIGHT, g_timestampSpaceshipEnd);

    glDeleteQueries(MAX_FRAMES_IN_FLIGHT, g_timestampViewAStart);
    glDeleteQueries(MAX_FRAMES_IN_FLIGHT, g_timestampViewAEnd);
    glDeleteQueries(MAX_FRAMES_IN_FLIGHT, g_timestampViewBStart);
    glDeleteQueries(MAX_FRAMES_IN_FLIGHT, g_timestampViewBEnd);
#endif

    return 0;
}
catch (std::exception const& eErr)
{
    std::fprintf(stderr, "Top-level Exception (%s):\n", typeid(eErr).name());
    std::fprintf(stderr, "%s\n", eErr.what());
    std::fprintf(stderr, "Bye.\n");
    return 1;
}


static void glfw_callback_error_(int aErrNum, char const* aErrDesc)
{
    std::fprintf(stderr, "GLFW error: %s (%d)\n", aErrDesc, aErrNum);
}

static void glfw_callback_key_(GLFWwindow* aWindow, int aKey, int, int aAction, int mods)
{
    if (GLFW_KEY_ESCAPE == aKey && GLFW_PRESS == aAction) {
        glfwSetWindowShouldClose(aWindow, GLFW_TRUE);
        return;
    }

    if (auto* state = static_cast<State_*>(glfwGetWindowUserPointer(aWindow))) {
        // Toggle split screen
        if (aKey == GLFW_KEY_V && aAction == GLFW_PRESS) {
            state->isSplitScreen = !state->isSplitScreen;
        }

        // Cycle camera1
        if (aKey == GLFW_KEY_C && (mods & GLFW_MOD_SHIFT) == 0 && aAction == GLFW_PRESS)
        {
            switch (state->cameraMode1)
            {
            case CameraMode::FREE:   state->cameraMode1 = CameraMode::CHASE;   break;
            case CameraMode::CHASE:  state->cameraMode1 = CameraMode::GROUND;  break;
            case CameraMode::GROUND: state->cameraMode1 = CameraMode::FREE;    break;
            }
        }

        // Cycle camera2
        if (aKey == GLFW_KEY_C && (mods & GLFW_MOD_SHIFT) && aAction == GLFW_PRESS)
        {
            switch (state->cameraMode2)
            {
            case CameraMode::CHASE:  state->cameraMode2 = CameraMode::GROUND;  break;
            case CameraMode::GROUND: state->cameraMode2 = CameraMode::CHASE;   break;
            default:                 state->cameraMode2 = CameraMode::CHASE;   break;
            }
        }

        // Animate rocket
        if (aKey == GLFW_KEY_F && aAction == GLFW_PRESS) {
            state->rcktCtrl.isMoving = !state->rcktCtrl.isMoving;
        }

        // Reload shaders & reset rocket
        if (aKey == GLFW_KEY_R && aAction == GLFW_PRESS)
        {
            if (state->prog)
            {
                state->rcktCtrl.reset();
                try {
                    state->prog->reload();
                    std::fprintf(stderr, "Shaders reloaded.\n");
                }
                catch (std::exception const& eErr) {
                    std::fprintf(stderr, "Error reloading shader:\n");
                    std::fprintf(stderr, "%s\n", eErr.what());
                }
            }
        }

        // WASD for camera1
        if (aKey == GLFW_KEY_W) {
            state->cam1.movingForward = (aAction != GLFW_RELEASE);
        }
        if (aKey == GLFW_KEY_S) {
            state->cam1.movingBack = (aAction != GLFW_RELEASE);
        }
        if (aKey == GLFW_KEY_A) {
            state->cam1.movingLeft = (aAction != GLFW_RELEASE);
        }
        if (aKey == GLFW_KEY_D) {
            state->cam1.movingRight = (aAction != GLFW_RELEASE);
        }
        if (aKey == GLFW_KEY_E) {
            state->cam1.movingUp = (aAction != GLFW_RELEASE);
        }
        if (aKey == GLFW_KEY_Q) {
            state->cam1.movingDown = (aAction != GLFW_RELEASE);
        }

        // SHIFT => speed up, CTRL => slow
        if (aKey == GLFW_KEY_LEFT_SHIFT || aKey == GLFW_KEY_RIGHT_SHIFT) {
            if (aAction == GLFW_PRESS)
                state->cam1.speed_multiplier = state->cam1.FAST_SPEED_MULT;
            else if (aAction == GLFW_RELEASE)
                state->cam1.speed_multiplier = state->cam1.NORMAL_SPEED_MULT;
        }
        if (aKey == GLFW_KEY_LEFT_CONTROL || aKey == GLFW_KEY_RIGHT_CONTROL) {
            if (aAction == GLFW_PRESS)
                state->cam1.speed_multiplier = state->cam1.SLOW_SPEED_MULT;
            else if (aAction == GLFW_RELEASE)
                state->cam1.speed_multiplier = state->cam1.NORMAL_SPEED_MULT;
        }
    }
}

static void glfw_callback_motion_(GLFWwindow* aWindow, double aX, double aY)
{
    if (auto* state = static_cast<State_*>(glfwGetWindowUserPointer(aWindow)))
    {
        if (state->cam1.cameraActive)
        {
            float dx = float(aX - state->cam1.lastX);
            float dy = float(aY - state->cam1.lastY);

            state->cam1.phi += dx * kMouseSensitivity_;
            state->cam1.theta += dy * kMouseSensitivity_;

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

static void mouse_button_callback(GLFWwindow* aWindow, int button, int action, int /*mods*/)
{
    if (auto* state = static_cast<State_*>(glfwGetWindowUserPointer(aWindow)))
    {
        if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
        {
            state->cam1.cameraActive = !state->cam1.cameraActive;
            if (state->cam1.cameraActive) {
                glfwSetInputMode(aWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
            else {
                glfwSetInputMode(aWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
        }
    }
}

static void updateCamera(State_::CamCtrl_& camera, float dt)
{
    float moveSpeed = kMovementPerSecond_ * dt * camera.speed_multiplier;

    camera.forward = {
        std::sin(camera.phi) * std::cos(camera.theta),
        -std::sin(camera.theta),
        -std::cos(camera.phi) * std::cos(camera.theta),
        0.f
    };
    camera.right = normalize(cross(camera.forward, Vec4f{ 0.f,1.f,0.f,0.f }));
    camera.up = cross(camera.right, camera.forward);

    Vec4f movement{ 0.f,0.f,0.f,0.f };
    if (camera.movingForward) movement += camera.forward * moveSpeed;
    if (camera.movingBack)    movement -= camera.forward * moveSpeed;
    if (camera.movingRight)   movement += camera.right * moveSpeed;
    if (camera.movingLeft)    movement -= camera.right * moveSpeed;
    if (camera.movingUp)      movement += camera.up * moveSpeed;
    if (camera.movingDown)    movement -= camera.up * moveSpeed;

    camera.position += movement;
}

static void updateRocket(State_::rcktCtrl_& rocket, float dt)
{
    if (rocket.isMoving)
    {
        Vec3f oldPos = rocket.position;
        rocket.time += dt;

        Vec3f newDirection = normalize(Vec3f(3.f, 1.f, -3.5f));
        Vec3f accelVec;
        if (rocket.time <= 5.f)
            accelVec = normalize(Vec3f(0.f, 1.f, 0.f));
        else
            accelVec = newDirection * rocket.acceleration;

        rocket.velocity += accelVec * dt;
        rocket.position += rocket.velocity * dt;

        Vec3f direction = rocket.position - oldPos;
        if (length(direction) > 0.001f) {
            direction = normalize(direction);
        }

        float pitch = atan2(direction.z, std::sqrt(direction.x * direction.x + direction.y * direction.y));
        float yaw = atan2(direction.x, direction.y);

        Mat44f rotPitch = make_rotation_x(pitch);
        Mat44f rotYaw = make_rotation_z(-yaw);
        Mat44f rotation = rotYaw * rotPitch;
        Mat44f translation = make_translation(rocket.position);

        rocket.model2worldRocket = translation * rotation;
    }
}

static Mat44f compute_view_matrix_for_camera(const State_::CamCtrl_& camCtrl,
    CameraMode mode,
    const State_& state)
{
    switch (mode)
    {
    case CameraMode::FREE:
        return make_look_at(camCtrl.position,
            camCtrl.position + camCtrl.forward,
            camCtrl.up);
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
    case CameraMode::GROUND:
    {
        auto const& rocketPos = state.rcktCtrl.position;
        Vec4f rocketPos4 = { rocketPos.x + 1.47f, rocketPos.y, rocketPos.z - 1.20f, 1.f };
        Vec4f groundPos4 = { state.groundCameraPos.x,
                             state.groundCameraPos.y,
                             state.groundCameraPos.z,
                             1.f
        };
        return make_look_at(groundPos4, rocketPos4, Vec4f{ 0.f,1.f,0.f,0.f });
    }
    }
    return kIdentity44f;
}

static GLuint setPointLights(State_::PointLight pointLights[MAX_POINT_LIGHTS],
    SimpleMeshData rocketPos)
{
    pointLights[0].position = rocketPos.pointLightPos[0];
    pointLights[0].color = { 1.f,0.f,0.f };
    pointLights[0].radius = 1.0f;
    pointLights[0].normals = rocketPos.pointLightNorms[0];

    pointLights[1].position = rocketPos.pointLightPos[1];
    pointLights[1].color = { 0.f,1.f,0.f };
    pointLights[1].radius = 1.0f;
    pointLights[1].normals = rocketPos.pointLightNorms[1];

    pointLights[2].position = rocketPos.pointLightPos[2];
    pointLights[2].color = { 0.f,0.f,1.f };
    pointLights[2].radius = 1.0f;
    pointLights[2].normals = rocketPos.pointLightNorms[2];

    GLuint pointLightUBO;
    glGenBuffers(1, &pointLightUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, pointLightUBO);

    State_::PointLightBlock block{};
    for (int i = 0; i < MAX_POINT_LIGHTS; i++) {
        block.lights[i] = pointLights[i];
    }

    glBufferData(GL_UNIFORM_BUFFER, sizeof(block), &block, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, pointLightUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    return pointLightUBO;
}

static void updatePointLights(Mat44f rocketPosition,
    SimpleMeshData rocketData,
    State_::PointLight pointLights[MAX_POINT_LIGHTS])
{
    Mat33f N = mat44_to_mat33(transpose(invert(rocketPosition)));
    for (int i = 0; i < MAX_POINT_LIGHTS; i++)
    {
        Vec4f posWS = rocketPosition * Vec4f{
            rocketData.pointLightPos[i].x,
            rocketData.pointLightPos[i].y,
            rocketData.pointLightPos[i].z,
            1.f
        };
        Vec3f normWS = normalize(N * rocketData.pointLightNorms[i]);
        pointLights[i].normals = normWS;
    }
}

static void updatePointLightUBO(GLuint pointLightUBO,
    State_::PointLight pointLights[MAX_POINT_LIGHTS])
{
    State_::PointLightBlock block{};
    for (int i = 0; i < MAX_POINT_LIGHTS; i++) {
        block.lights[i] = pointLights[i];
    }
    glBindBuffer(GL_UNIFORM_BUFFER, pointLightUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(block), &block);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

static void renderScene(const State_& state,
    const Mat44f& view,
    const Mat44f& projection,
    GLuint langersoVao, const SimpleMeshData& langersoMesh, GLuint langersoTextureId, size_t langersoCount,
    GLuint rocketVao, const SimpleMeshData& rocketMesh, size_t rocketCount,
    GLuint launchpadVao, const SimpleMeshData& launchpadMesh, size_t launchpadCount)
{
    glUseProgram(state.prog->programId());

    // Simple directional light
    Vec3f lightDir = normalize(Vec3f{ 0.f,1.f,-1.f });
    glUniform3fv(2, 1, &lightDir.x);
    glUniform3f(3, 0.678f, 0.847f, 0.902f);
    glUniform3f(4, 0.05f, 0.05f, 0.05f);

    // Terrain
#ifdef ENABLE_PERFORMANCE_METRICS
    glQueryCounter(g_timestampTerrainStart[g_currentFrameIndex], GL_TIMESTAMP);
#endif

    {
        Mat44f model2world = kIdentity44f;
        Mat33f normalMatrix = mat44_to_mat33(transpose(invert(model2world)));
        Mat44f mvp = projection * view * model2world;

        glUniformMatrix4fv(0, 1, GL_TRUE, mvp.v);
        glUniformMatrix3fv(1, 1, GL_TRUE, normalMatrix.v);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, langersoTextureId);
        glUniform1i(5, langersoMesh.isTextureSupplied);

        glUniform2f(6, langersoMesh.mins.x, langersoMesh.mins.y);
        glUniform2f(7, langersoMesh.diffs.x, langersoMesh.diffs.y);

        glBindVertexArray(langersoVao);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)langersoCount);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
#ifdef ENABLE_PERFORMANCE_METRICS
    glQueryCounter(g_timestampTerrainEnd[g_currentFrameIndex], GL_TIMESTAMP);
#endif

    // Spaceship
#ifdef ENABLE_PERFORMANCE_METRICS
    glQueryCounter(g_timestampSpaceshipStart[g_currentFrameIndex], GL_TIMESTAMP);
#endif

    {
        Mat44f model2world = state.rcktCtrl.model2worldRocket;
        Mat33f normalMatrix = mat44_to_mat33(transpose(invert(model2world)));
        Mat44f mvp = projection * view * model2world;

        glUniformMatrix4fv(0, 1, GL_TRUE, mvp.v);
        glUniformMatrix3fv(1, 1, GL_TRUE, normalMatrix.v);
        glUniform1i(5, rocketMesh.isTextureSupplied);

        glBindVertexArray(rocketVao);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)rocketCount);
    }
#ifdef ENABLE_PERFORMANCE_METRICS
    glQueryCounter(g_timestampSpaceshipEnd[g_currentFrameIndex], GL_TIMESTAMP);
#endif

    // Launchpad #1
#ifdef ENABLE_PERFORMANCE_METRICS
    glQueryCounter(g_timestampLaunchpadsStart[g_currentFrameIndex], GL_TIMESTAMP);
#endif

    {
        Mat44f model2world = kIdentity44f;
        Mat33f normalMatrix = mat44_to_mat33(transpose(invert(model2world)));
        Mat44f mvp = projection * view * model2world;

        glUniformMatrix4fv(0, 1, GL_TRUE, mvp.v);
        glUniformMatrix3fv(1, 1, GL_TRUE, normalMatrix.v);
        glUniform1i(5, launchpadMesh.isTextureSupplied);

        glBindVertexArray(launchpadVao);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)launchpadCount);
    }

    // Launchpad #2
    {
        Mat44f model2world = make_translation({ 3.f,0.f,-5.f });
        Mat33f normalMatrix = mat44_to_mat33(transpose(invert(model2world)));
        Mat44f mvp = projection * view * model2world;

        glUniformMatrix4fv(0, 1, GL_TRUE, mvp.v);
        glUniformMatrix3fv(1, 1, GL_TRUE, normalMatrix.v);
        glUniform1i(5, launchpadMesh.isTextureSupplied);

        glBindVertexArray(launchpadVao);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)launchpadCount);
    }
#ifdef ENABLE_PERFORMANCE_METRICS
    glQueryCounter(g_timestampLaunchpadsEnd[g_currentFrameIndex], GL_TIMESTAMP);
#endif
}
