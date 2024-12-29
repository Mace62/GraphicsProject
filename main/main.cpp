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


#if defined(_WIN32) // alternative: #if defined(_MSC_VER)
extern "C"
{
	__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
	__declspec(dllexport) unsigned long AmdPowerXpressRequestHighPerformance = 1; // untested
}
#endif

// Define all asset paths here
const std::string DIR_PATH = std::filesystem::current_path().string();

const std::string LANGERSO_OBJ_ASSET_PATH = DIR_PATH + "/assets/cw2/langerso.obj";
const std::string LANGERSO_TEXTURE_ASSET_PATH = DIR_PATH + "/assets/cw2/L3211E-4k.jpg";
const std::string LAUNCHPAD_OBJ_ASSET_PATH = DIR_PATH + "/assets/cw2/landingpad.obj";

const int MAX_POINT_LIGHTS = 3;
constexpr Vec3f rocketStartPos = { 0.0f, 0.0f, 0.0f };

namespace
{
	constexpr char const* kWindowTitle = "COMP3811 - CW2";
	
	constexpr float kMovementPerSecond_ = 5.f; // units per second
	constexpr float kMouseSensitivity_ = 0.01f; // radians per pixel

	constexpr float rocketAcceleration_ = 0.1f;


	enum class CameraMode {
		FREE = 0,
		CHASE,
		GROUND
	};

	struct State_
	{
		ShaderProgram* prog;

		CameraMode currentCameraMode = CameraMode::FREE;

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

			// Add vectors for camera orientation
			Vec4f position = { 0.0f, 5.0f, 0.0f, 1.0f };
			Vec4f forward = { 0.0f, 0.0f, -1.0f, 0.0f };
			Vec4f right = { 1.0f, 0.0f, 0.0f, 0.0f };
			Vec4f up = { 0.0f, 1.0f, 0.0f, 0.0f };

			bool cameraActive;
			bool actionZoomIn, actionZoomOut;

			float phi, theta;
			float radius;
			float speed_multiplier = NORMAL_SPEED_MULT;

			float lastX, lastY;
			float lastTheta;
		} camControl;

		// Array to hold multiple point lights
		struct PointLight {
			Vec3f position;    // vec3
			float padding1;    // 4 bytes of padding for std140 alignment
			Vec3f color;       // vec3
			float padding2;    // 4 bytes of padding for std140 alignment
			Vec3f normals;     // vec3
			float radius;      // float, aligned to 4 bytes
		};

		struct PointLightBlock {
			PointLight lights[MAX_POINT_LIGHTS];
		};

		struct rcktCtrl_ {
			Vec3f position = rocketStartPos; // Starting position
			Vec3f velocity = { 0.0f, 0.0f, 0.0f };   // Velocity starts at zero
			Mat44f model2worldRocket = kIdentity44f;	// No initial movements or acceleration
			float acceleration = rocketAcceleration_;                   // Acceleration factor
			float time = 0.0f;                           // Time for curved path calculation
			bool isMoving = false;                       // Movement state
			float pitch = 0.f;
			float yaw = 0.f;

			Mat44f translationMatrix;
			Mat44f rotation;

			void reset() {
				model2worldRocket = kIdentity44f;
				position = rocketStartPos;
				velocity = { 0.0f, 0.0f, 0.0f };
				acceleration = rocketAcceleration_;
				time = 0.0f;
				isMoving = false;
				pitch = 0.0f;
			}
		} rcktCtrl;

		float chaseDistance = 1.0f;    // distance behind the rocket
		Vec3f groundCameraPos = { -5.f, 1.0f, 0.f }; // chosen ground location
	};

	void glfw_callback_error_(int, char const*);

	void glfw_callback_key_(GLFWwindow*, int, int, int, int);
	void glfw_callback_motion_(GLFWwindow*, double, double);
	void mouse_button_callback(GLFWwindow* aWindow, int button, int action, int mods);

	void updateCamera(State_::CamCtrl_& camera, float dt);
	void updateRocket(State_::rcktCtrl_& rocket, float dt);
	Mat44f compute_view_matrix(State_ const& state);


	GLuint setPointLights(State_::PointLight pointLights[MAX_POINT_LIGHTS], SimpleMeshData rocketPos);
	void updatePointLights(Mat44f rocketPosition, SimpleMeshData rocketPos, State_::PointLight pointLights[MAX_POINT_LIGHTS]);
	void updatePointLightUBO(GLuint pointLightUBO, State_::PointLight pointLights[MAX_POINT_LIGHTS]);


	struct GLFWCleanupHelper
	{
		~GLFWCleanupHelper();
	};
	struct GLFWWindowDeleter
	{
		~GLFWWindowDeleter();
		GLFWwindow* window;
	};
}

int main() try
{
	// Initialize GLFW
	if( GLFW_TRUE != glfwInit() )
	{
		char const* msg = nullptr;
		int ecode = glfwGetError( &msg );
		throw Error( "glfwInit() failed with '%s' (%d)", msg, ecode );
	}

	// Ensure that we call glfwTerminate() at the end of the program.
	GLFWCleanupHelper cleanupHelper;

	// Configure GLFW and create window
	glfwSetErrorCallback( &glfw_callback_error_ );

	glfwWindowHint( GLFW_SRGB_CAPABLE, GLFW_TRUE );
	glfwWindowHint( GLFW_DOUBLEBUFFER, GLFW_TRUE );

	//glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );

#	if !defined(__APPLE__)
	// Most platforms will support OpenGL 4.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#	else // defined(__APPLE__)
	// Apple has at most OpenGL 4.1, so don't ask for something newer.
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
#	endif // ~ __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#	if !defined(NDEBUG)
	// When building in debug mode, request an OpenGL debug context. This
	// enables additional debugging features. However, this can carry extra
	// overheads. We therefore do not do this for release builds.
	glfwWindowHint( GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE );
#	endif // ~ !NDEBUG

	GLFWwindow* window = glfwCreateWindow(
		1280,
		720,
		kWindowTitle,
		nullptr, nullptr
	);

	if( !window )
	{
		char const* msg = nullptr;
		int ecode = glfwGetError( &msg );
		throw Error( "glfwCreateWindow() failed with '%s' (%d)", msg, ecode );
	}

	GLFWWindowDeleter windowDeleter{ window };


	// Set up event handling
	State_ state{};

	glfwSetWindowUserPointer(window, &state);

	glfwSetKeyCallback(window, &glfw_callback_key_);
	glfwSetCursorPosCallback(window, &glfw_callback_motion_);
	glfwSetMouseButtonCallback(window, &mouse_button_callback);

	// Set up drawing stuff
	glfwMakeContextCurrent( window );
	glfwSwapInterval( 1 ); // V-Sync is on.

	// Initialize GLAD
	// This will load the OpenGL API. We mustn't make any OpenGL calls before this!
	if( !gladLoadGLLoader( (GLADloadproc)&glfwGetProcAddress ) )
		throw Error( "gladLoaDGLLoader() failed - cannot load GL API!" );

	std::printf( "RENDERER %s\n", glGetString( GL_RENDERER ) );
	std::printf( "VENDOR %s\n", glGetString( GL_VENDOR ) );
	std::printf( "VERSION %s\n", glGetString( GL_VERSION ) );
	std::printf( "SHADING_LANGUAGE_VERSION %s\n", glGetString( GL_SHADING_LANGUAGE_VERSION ) );

	// Ddebug output
#	if !defined(NDEBUG)
	setup_gl_debug_output();
#	endif // ~ !NDEBUG

	// Global GL state
	OGL_CHECKPOINT_ALWAYS();

	glClearColor(0.2f, 0.2f, 0.2f, 0.0f);

	// Enable FRAMEBUFFER_SRGB for gamma-correct rendering
	glEnable(GL_FRAMEBUFFER_SRGB);

	// Enable depth testing
	glEnable(GL_DEPTH_TEST);

	// Enable face culling
	//glEnable(GL_CULL_FACE);        // Enable face culling
	//glCullFace(GL_BACK);           // Cull back faces (default)
	//glFrontFace(GL_CCW);           // Counter-clockwise vertices define front faces

	OGL_CHECKPOINT_ALWAYS();

	// Get actual framebuffer size.
	// This can be different from the window size, as standard window
	// decorations (title bar, borders, ...) may be included in the window size
	// but not be part of the drawable surface area.
	int iwidth, iheight;
	glfwGetFramebufferSize( window, &iwidth, &iheight );

	glViewport( 0, 0, iwidth, iheight );

	// Load shader program
	ShaderProgram prog({
		{ GL_VERTEX_SHADER, "assets/cw2/default.vert" },
		{ GL_FRAGMENT_SHADER, "assets/cw2/default.frag" }
	});

	state.prog = &prog;
	state.camControl.radius = 10.f;

	// Animation state
	auto last = Clock::now();

	float angle = 0.f;

	// Other initialization & loading
	OGL_CHECKPOINT_ALWAYS();
	
	// Load Langerso mesh
	auto langersoMesh = load_wavefront_obj(LANGERSO_OBJ_ASSET_PATH.c_str(), true);
	GLuint langersoVao = create_vao(langersoMesh);
	GLuint langersoTextureObjectId = load_texture_2d(LANGERSO_TEXTURE_ASSET_PATH.c_str());
	std::size_t langersoVertexCount = langersoMesh.positions.size();

	// Load launchpad mesh
	auto launchpadMesh = load_wavefront_obj(LAUNCHPAD_OBJ_ASSET_PATH.c_str(), false,
		make_translation({ 2.f, 0.005f, -2.f }) * make_scaling(0.5f, 0.5f, 0.5f)
	);
	GLuint launchpadVao = create_vao(launchpadMesh);
	std::size_t launchpadVertexCount = launchpadMesh.positions.size();

	// Load rocket mesh
    auto rocketMesh = create_spaceship(32,			// Subdivs
		{0.2f, 0.2f, 0.2f}, {0.8f, 0.2f, 0.2f},		// Colours for rocket body and fins
		make_translation({ 2.f, 0.15f, -2.f }) * make_scaling(0.05f, 0.05f, 0.05f),		// Pretransform matrix
		false
	);
    GLuint rocketVao = create_vao(rocketMesh);
    std::size_t rocketVertexCount = rocketMesh.positions.size();

	// Set point lights
	State_::PointLight pointLights[MAX_POINT_LIGHTS];
	GLuint pointLightUBO = setPointLights(pointLights, rocketMesh);

    OGL_CHECKPOINT_ALWAYS();

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Let GLFW process events
        glfwPollEvents();

        // Check if window was resized.
        float fbwidth, fbheight;
        {
            int nwidth, nheight;
            glfwGetFramebufferSize(window, &nwidth, &nheight);

            fbwidth = float(nwidth);
            fbheight = float(nheight);

            if (0 == nwidth || 0 == nheight)
            {
                // Window minimized? Pause until it is unminimized.
                do
                {
                    glfwWaitEvents();
                    glfwGetFramebufferSize(window, &nwidth, &nheight);
                } while (0 == nwidth || 0 == nheight);
            }

            glViewport(0, 0, nwidth, nheight);
        }

		// Update state
		//TODO: update state of rocket
		auto const now = Clock::now();
		float dt = std::chrono::duration_cast<Secondsf>(now - last).count();
		last = now;

		angle += dt * std::numbers::pi_v<float> *0.3f;
		if (angle >= 2.f * std::numbers::pi_v<float>)
			angle -= 2.f * std::numbers::pi_v<float>;

		// Update camera state
		if (state.camControl.actionZoomIn)
			state.camControl.radius -= kMovementPerSecond_ * dt;
		else if (state.camControl.actionZoomOut)
			state.camControl.radius += kMovementPerSecond_ * dt;

		if (state.camControl.radius <= 0.1f)
			state.camControl.radius = 0.1f;

		// Projection matrix setup
		Mat44f projection = make_perspective_projection(
			60.f * std::numbers::pi_v<float> / 180.f,  // FOV: 60 degrees
			fbwidth / float(fbheight),                 // Aspect ratio
			0.1f, 100.0f                              // Near and far planes
		);


		// Always update the FREE camera’s internal orientation, even if it’s not currently in FREE mode.
		// (So when the user returns to FREE mode, the orientation is up-to-date.)
		updateCamera(state.camControl, dt);

		Mat44f world2camera = compute_view_matrix(state);


		

		// Map Langerso model to world
		Mat44f model2worldLangerso = kIdentity44f;
		Mat33f normalMatrixLangerso = mat44_to_mat33(transpose(invert(model2worldLangerso)));
		Mat44f projCameraWorldLangerso = projection * world2camera * model2worldLangerso;

		// Map Launcpad 1 model to world
		Mat44f model2worldLaunchpad1 = kIdentity44f;
		Mat33f normalMatrixLaunchpad1 = mat44_to_mat33(transpose(invert(model2worldLaunchpad1)));
		Mat44f projCameraWorldLaunchpad1 = projection * world2camera * model2worldLaunchpad1;

		// Map Launcpad 2 model to world
		Mat44f model2worldLaunchpad2 = make_translation({ 3.f, 0.f, -5.f });
		Mat33f normalMatrixLaunchpad2 = mat44_to_mat33(transpose(invert(model2worldLaunchpad2)));
		Mat44f projCameraWorldLaunchpad2 = projection * world2camera * model2worldLaunchpad2;


		// Map Rocket model to world
		updateRocket(state.rcktCtrl, dt);

		// Update point light positions
		updatePointLights(state.rcktCtrl.model2worldRocket, rocketMesh, pointLights);

		// Update the UBO with the new point light data
		updatePointLightUBO(pointLightUBO, pointLights);
			
		Mat33f normalMatrixRocket = mat44_to_mat33(transpose(invert(state.rcktCtrl.model2worldRocket)));
		Mat44f projCameraWorldRocket = projection * world2camera * state.rcktCtrl.model2worldRocket;


		// Draw scene
		OGL_CHECKPOINT_DEBUG();

		// Clear color buffer to specified clear color (glClearColor())
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// We want to draw with our program.
		glUseProgram(prog.programId());

		// Task 2 general light dir requirement (needs to be applied to ALL objects)
		Vec3f lightDir = normalize(Vec3f{ 0.f, 1.f, -1.f });

		glUniform3fv(2, 1, &lightDir.x); // Apply light dir vec
		glUniform3f(3, 0.678f, 0.847f, 0.902f);	// Apply diffuse vec (light blue tint)
		glUniform3f(4, 0.05f, 0.05f, 0.05f);	// Apply scene ambience vec


		/*	FOR SCENE MESH	*/
		// Parse Normalisation matrix to vertex shader for langerso model
		glUniformMatrix3fv(
			1, // make sure this matches the location = N in the vertex shader!
			1, GL_TRUE, normalMatrixLangerso.v
		);

		// Bind textures
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, langersoTextureObjectId);
		glUniform1i(5, langersoMesh.isTextureSupplied);

		// Give min and dims of the mesh to align tex coords with mesh 
		glUniform2f(6, langersoMesh.mins.x, langersoMesh.mins.y);
		glUniform2f(7, langersoMesh.diffs.x, langersoMesh.diffs.y);

		// Draw scene
		glUniformMatrix4fv(0, 1, GL_TRUE, projCameraWorldLangerso.v);
		glBindVertexArray(langersoVao);
		glDrawArrays(GL_TRIANGLES, 0, langersoVertexCount);

		// Unbind and reset texture to stop it being applied to other models
		glBindTexture(GL_TEXTURE_2D, 0);



		/*	FOR ROCKET MESH	*/
		// Parse Normalisation matrix to vertex shader for rocket
		glUniformMatrix3fv(
			1, // make sure this matches the location = N in the vertex shader!
			1, GL_TRUE, normalMatrixRocket.v
		);

		// Draw rocket
		glUniform1i(5, rocketMesh.isTextureSupplied);
        glUniformMatrix4fv(0, 1, GL_TRUE, projCameraWorldRocket.v);
        glBindVertexArray(rocketVao);
        glDrawArrays(GL_TRIANGLES, 0, rocketVertexCount);



		/*	FOR LAUNCHPAD MESH	*/
		// Parse Normalisation matrix to vertex shader for launchpad 1
		glUniformMatrix3fv(
			1, // make sure this matches the location = N in the vertex shader!
			1, GL_TRUE, normalMatrixLaunchpad1.v
		);

		// Draw launchpad 1
		glUniform1i(5, launchpadMesh.isTextureSupplied);
		glUniformMatrix4fv(0, 1, GL_TRUE, projCameraWorldLaunchpad1.v);
		glBindVertexArray(launchpadVao);
		glDrawArrays(GL_TRIANGLES, 0, launchpadVertexCount);


		// Parse Normalisation matrix to vertex shader for launchpad 2
		glUniformMatrix3fv(
			1, // make sure this matches the location = N in the vertex shader!
			1, GL_TRUE, normalMatrixLaunchpad2.v
		);

		// Draw launchpad 2
		glUniform1i(5, launchpadMesh.isTextureSupplied);
		glUniformMatrix4fv(0, 1, GL_TRUE, projCameraWorldLaunchpad2.v);
		glBindVertexArray(launchpadVao);
		glDrawArrays(GL_TRIANGLES, 0, launchpadVertexCount);



        OGL_CHECKPOINT_DEBUG();

		// Display results
		glfwSwapBuffers( window );
	}

	// Cleanup.
	state.prog = nullptr;
	
	//TODO: additional cleanup
	
	return 0;
}
catch( std::exception const& eErr )
{
	std::fprintf( stderr, "Top-level Exception (%s):\n", typeid(eErr).name() );
	std::fprintf( stderr, "%s\n", eErr.what() );
	std::fprintf( stderr, "Bye.\n" );
	return 1;
}


namespace
{
	void glfw_callback_error_(int aErrNum, char const* aErrDesc)
	{
		std::fprintf(stderr, "GLFW error: %s (%d)\n", aErrDesc, aErrNum);
	}

	void glfw_callback_key_(GLFWwindow* aWindow, int aKey, int, int aAction, int)
	{
		if (GLFW_KEY_ESCAPE == aKey && GLFW_PRESS == aAction)
		{
			glfwSetWindowShouldClose(aWindow, GLFW_TRUE);
			return;
		}

		if (auto* state = static_cast<State_*>(glfwGetWindowUserPointer(aWindow)))
		{
			// Press 'C' to cycle cameras
			if (aKey == GLFW_KEY_C && aAction == GLFW_PRESS)
			{
				switch (state->currentCameraMode)
				{
				case CameraMode::FREE:
					state->currentCameraMode = CameraMode::CHASE;
					break;
				case CameraMode::CHASE:
					state->currentCameraMode = CameraMode::GROUND;
					break;
				case CameraMode::GROUND:
					state->currentCameraMode = CameraMode::FREE;
					break;
				}
			}

			// Start rocket animation with 'F'
			if (GLFW_KEY_F == aKey && GLFW_PRESS == aAction) {
				state->rcktCtrl.isMoving = !state->rcktCtrl.isMoving;
			}
			// R-key reloads shaders.
			if (GLFW_KEY_R == aKey && GLFW_PRESS == aAction)
			{
				if (state->prog)
				{
					state->rcktCtrl.reset();
					try
					{
						state->prog->reload();
						std::fprintf(stderr, "Shaders reloaded and recompiled.\n");
					}
					catch (std::exception const& eErr)
					{
						std::fprintf(stderr, "Error when reloading shader:\n");
						std::fprintf(stderr, "%s\n", eErr.what());
						std::fprintf(stderr, "Keeping old shader.\n");
					}
				}
			}

			// Handle WASD keys
			if (GLFW_KEY_W == aKey)
			{
				state->camControl.movingForward = (aAction != GLFW_RELEASE);
			}
			if (GLFW_KEY_S == aKey)
			{
				state->camControl.movingBack = (aAction != GLFW_RELEASE);
			}
			if (GLFW_KEY_A == aKey)
			{
				state->camControl.movingLeft = (aAction != GLFW_RELEASE);
			}
			if (GLFW_KEY_D == aKey)
			{
				state->camControl.movingRight = (aAction != GLFW_RELEASE);
			}
			if (GLFW_KEY_E == aKey)
			{
				state->camControl.movingUp = (aAction != GLFW_RELEASE);
			}
			if (GLFW_KEY_Q == aKey)
			{
				state->camControl.movingDown = (aAction != GLFW_RELEASE);
			}

			if (aKey == GLFW_KEY_LEFT_SHIFT || aKey == GLFW_KEY_RIGHT_SHIFT) {
				if (aAction == GLFW_PRESS) {
					state->camControl.speed_multiplier = state->camControl.FAST_SPEED_MULT;
				}
				else if (aAction == GLFW_RELEASE) {
					state->camControl.speed_multiplier = state->camControl.NORMAL_SPEED_MULT; // Reset to normal speed when released
				}
			}

			if (aKey == GLFW_KEY_LEFT_CONTROL || aKey == GLFW_KEY_RIGHT_CONTROL) {
				if (aAction == GLFW_PRESS) {
					state->camControl.speed_multiplier = state->camControl.SLOW_SPEED_MULT;
				}
				else if (aAction == GLFW_RELEASE) {
					state->camControl.speed_multiplier = state->camControl.NORMAL_SPEED_MULT; // Reset to normal speed when released
				}
			}
		}
	}

	void glfw_callback_motion_(GLFWwindow* aWindow, double aX, double aY)
	{
		if (auto* state = static_cast<State_*>(glfwGetWindowUserPointer(aWindow)))
		{
			if (state->camControl.cameraActive)
			{
				auto const dx = float(aX - state->camControl.lastX);
				auto const dy = float(aY - state->camControl.lastY);

				state->camControl.phi += dx * kMouseSensitivity_;

				state->camControl.theta += dy * kMouseSensitivity_;
				if (state->camControl.theta > std::numbers::pi_v<float> / 2.f)
					state->camControl.theta = state->camControl.lastTheta;
				else if (state->camControl.theta < -std::numbers::pi_v<float> / 2.f)
					state->camControl.theta = state->camControl.lastTheta;
			}

			state->camControl.lastX = float(aX);
			state->camControl.lastY = float(aY);
			state->camControl.lastTheta = state->camControl.theta;
		}
	}
	void mouse_button_callback(GLFWwindow* aWindow, int button, int action, int /*mods*/)
	{
		if (auto* state = static_cast<State_*>(glfwGetWindowUserPointer(aWindow)))
		{
			if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
			{
				state->camControl.cameraActive = !state->camControl.cameraActive;

				if (state->camControl.cameraActive)
					glfwSetInputMode(aWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Cursor disabled - hidden and locked to the centre of the screen
				else
					glfwSetInputMode(aWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
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


	Mat44f compute_view_matrix(State_ const& state)
	{
		switch (state.currentCameraMode)
		{
			// (1) Free camera
		case CameraMode::FREE:
		{
			auto const& cam = state.camControl;
			return make_look_at(
				cam.position,
				cam.position + cam.forward,
				cam.up
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
			rocket.model2worldRocket = translationMatrix * rotationMatrix;


			// Debug output for verification
			/*printf("Rocket Time: %f\n", rocket.time);
			printf("Position: (%f, %f, %f)\n", rocket.position.x, rocket.position.y, rocket.position.z);
			printf("Velocity: (%f, %f, %f)\n", rocket.velocity.x, rocket.velocity.y, rocket.velocity.z);
			printf("Direction: (%f, %f, %f)\n", direction.x, direction.y, direction.z);*/
		}
	}

}


namespace
{
	GLFWCleanupHelper::~GLFWCleanupHelper()
	{
		glfwTerminate();
	}

	GLFWWindowDeleter::~GLFWWindowDeleter()
	{
		if( window )
			glfwDestroyWindow( window );
	}
}
