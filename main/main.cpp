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


#if defined(_WIN32) // alternative: ”#if defined(_MSC_VER)”
extern "C"
{
	__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
	__declspec(dllexport) unsigned long AmdPowerXpressRequestHighPerformance = 1; // untested
	// See https://stackoverflow.com/questions/17458803/amd-equivalent-to-nvoptimusenablement
}
#endif

// Define all asset paths here
const std::string DIR_PATH = std::filesystem::current_path().string();
const std::string LANGERSO_OBJ_ASSET_PATH = DIR_PATH + "/assets/cw2/langerso.obj";


namespace
{
	constexpr char const* kWindowTitle = "COMP3811 - CW2";
	
	constexpr float kMovementPerSecond_ = 5.f; // units per second
	constexpr float kMouseSensitivity_ = 0.01f; // radians per pixel

	struct State_
	{
		ShaderProgram* prog;

		struct CamCtrl_
		{
			float FAST_SPEED_MULT = 4.f;
			float SLOW_SPEED_MULT = 0.25f;
			float NORMAL_SPEED_MULT = 1.f;

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
	};

	void glfw_callback_error_(int, char const*);

	void glfw_callback_key_(GLFWwindow*, int, int, int, int);
	void glfw_callback_motion_(GLFWwindow*, double, double);
	void mouse_button_callback(GLFWwindow* aWindow, int button, int action, int mods);
	void updateCamera(State_::CamCtrl_& camera, float dt);

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
	auto langersoMesh = load_wavefront_obj(LANGERSO_OBJ_ASSET_PATH.c_str());
	GLuint langersoVao = create_vao(langersoMesh);
	std::size_t langersoVertexCount = langersoMesh.positions.size();

	OGL_CHECKPOINT_ALWAYS();

	// Main loop
	while( !glfwWindowShouldClose( window ) )
	{
		// Let GLFW process events
		glfwPollEvents();
		
		// Check if window was resized.
		float fbwidth, fbheight;
		{
			int nwidth, nheight;
			glfwGetFramebufferSize( window, &nwidth, &nheight );

			fbwidth = float(nwidth);
			fbheight = float(nheight);

			if( 0 == nwidth || 0 == nheight )
			{
				// Window minimized? Pause until it is unminimized.
				// This is a bit of a hack.
				do
				{
					glfwWaitEvents();
					glfwGetFramebufferSize( window, &nwidth, &nheight );
				} while( 0 == nwidth || 0 == nheight );
			}

			glViewport( 0, 0, nwidth, nheight );
		}

		// Update state
		//TODO: update state
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

		// Update camera position with new FPS camera system
		updateCamera(state.camControl, dt);

		// Calculate new view matrix using look_at func
		Mat44f world2camera = make_look_at(
			state.camControl.position,
			state.camControl.position + state.camControl.forward,
			state.camControl.up
		);

		// Map model to world
		Mat44f model2world = kIdentity44f;

		Mat33f normalMatrix = mat44_to_mat33(transpose(invert(model2world)));

		Mat44f projCameraWorldLangerso = projection * world2camera * model2world;


		// Draw scene
		OGL_CHECKPOINT_DEBUG();

		// Clear color buffer to specified clear color (glClearColor())
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// We want to draw with our program.
		glUseProgram(prog.programId());

		// Parse Normalisation matrix to vertex shader
		glUniformMatrix3fv(
			1, // make sure this matches the location = N in the vertex shader!
			1, GL_TRUE, normalMatrix.v
		);

		// Parse Normalisation matrix to vertex shader
		glUniformMatrix3fv(
			1, // make sure this matches the location = N in the vertex shader!
			1, GL_TRUE, normalMatrix.v
		);

		// Task 2 general light dir requirement (needs to be applied to ALL objects)
		Vec3f lightDir = normalize(Vec3f{ 0.f, 1.f, -1.f });

		glUniform3fv(2, 1, &lightDir.x); // Apply light dir vec
		glUniform3f(3, 0.678f, 0.847f, 0.902f);	// Apply diffuse vec (light blue tint)
		glUniform3f(4, 0.05f, 0.05f, 0.05f);	// Apply scene ambience vec

		// Draw scene
		glUniformMatrix4fv(0, 1, GL_TRUE, projCameraWorldLangerso.v);
		glBindVertexArray(langersoVao);
		glDrawArrays(GL_TRIANGLES, 0, langersoVertexCount);

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
			// R-key reloads shaders.
			if (GLFW_KEY_R == aKey && GLFW_PRESS == aAction)
			{
				if (state->prog)
				{
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

			// Space toggles camera
			if (GLFW_KEY_SPACE == aKey && GLFW_PRESS == aAction)
			{
				state->camControl.cameraActive = !state->camControl.cameraActive;

				if (state->camControl.cameraActive)
					glfwSetInputMode(aWindow, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
				else
					glfwSetInputMode(aWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
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
			}
		}
	}

	// Add this function to update camera position based on movement
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
