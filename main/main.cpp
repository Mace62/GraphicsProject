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

//TODO: REMOVE THIS
#include "triangle_prism.hpp"

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

constexpr Vec3f rocketStartPos = { 2.f, 0.15f, -2.f };


namespace
{
	constexpr char const* kWindowTitle = "COMP3811 - CW2";
	
	constexpr float kMovementPerSecond_ = 5.f; // units per second
	constexpr float kMouseSensitivity_ = 0.01f; // radians per pixel

	constexpr float rocketAcceleration_ = 0.01f;

	struct State_
	{
		ShaderProgram* prog;

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

		struct rcktCtrl_ {
			Vec3f position = rocketStartPos; // Starting position
			Vec3f velocity = { 0.0f, 0.0f, 0.0f };   // Velocity starts at zero
			float acceleration = rocketAcceleration_;                   // Acceleration factor
			float time = 0.0f;                           // Time for curved path calculation
			bool isMoving = false;                       // Movement state
			float pitch = 0.f;       

			void reset() {
				position = rocketStartPos;
				velocity = { 0.0f, 0.0f, 0.0f };
				acceleration = rocketAcceleration_;
				time = 0.0f;
				isMoving = false;
				pitch = 0.0f;
			}
		} rcktCtrl;
	};

	void glfw_callback_error_(int, char const*);

	void glfw_callback_key_(GLFWwindow*, int, int, int, int);
	void glfw_callback_motion_(GLFWwindow*, double, double);
	void mouse_button_callback(GLFWwindow* aWindow, int button, int action, int mods);
	void updateCamera(State_::CamCtrl_& camera, float dt);
	void updateRocket(State_::rcktCtrl_& rocket, float dt, Mat44f& model2worldRocket);

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
		{0.2f, 0.2f, 0.2f}, {1.f, 0.2f, 0.2f},		// Colours for rocket body and fins
		make_scaling(0.05f, 0.05f, 0.05f), 		// Pretransform matrix
		false
	);
    GLuint rocketVao = create_vao(rocketMesh);
    std::size_t rocketVertexCount = rocketMesh.positions.size();

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


		// Update camera position with new FPS camera system
		updateCamera(state.camControl, dt);

		// Calculate new view matrix using look_at func
		Mat44f world2camera = make_look_at(
			state.camControl.position,
			state.camControl.position + state.camControl.forward,
			state.camControl.up
		);

		// Map Langerso model to world
		Mat44f model2worldLangerso = kIdentity44f;
		Mat33f normalMatrixLangerso = mat44_to_mat33(transpose(invert(model2worldLangerso)));
		Mat44f projCameraWorldLangerso = projection * world2camera * model2worldLangerso;

		// Map Launcpad 1 model to world
		Mat44f model2worldLaunchpad1 = kIdentity44f;
		Mat33f normalMatrixLaunchpad1 = mat44_to_mat33(transpose(invert(model2worldLaunchpad1)));
		Mat44f projCameraWorldLaunchpad1 = projection * world2camera * model2worldLaunchpad1;

		// Map Launcpad 1 model to world
		Mat44f model2worldLaunchpad2 = make_translation({ 3.f, 0.f, -5.f });
		Mat33f normalMatrixLaunchpad2 = mat44_to_mat33(transpose(invert(model2worldLaunchpad2)));
		Mat44f projCameraWorldLaunchpad2 = projection * world2camera * model2worldLaunchpad2;


		// Map Rocket model to world
		// TODO: CHANGE THIS WHEN CONSTRUCTING ANIMATION TO REFLECT UPDATED POS OF ROCKET
		//Mat44f model2worldRocket = kIdentity44f;
		//Mat44f rotationY = make_rotation_y(state.rcktCtrl.yaw);   // Yaw (around Y-axis)
		//Mat44f rotationX = make_rotation_x(state.rcktCtrl.pitch); // Pitch (around X-axis)

		//// Combine rotations: Apply pitch first, then yaw.
		//Mat44f finalRotation = kIdentity44f; //= rotationY * rotationX;

		/*Mat44f model2worldRocket = finalRotation * make_translation(state.rcktCtrl.position);*/

		Mat44f model2worldRocket = kIdentity44f * make_translation(rocketStartPos);

		updateRocket(state.rcktCtrl, dt, model2worldRocket);
			
		Mat33f normalMatrixRocket = mat44_to_mat33(transpose(invert(model2worldRocket)));
		Mat44f projCameraWorldRocket = projection * world2camera * model2worldRocket;


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
			// Start rocket animation with 'F'
			if (GLFW_KEY_F == aKey && GLFW_PRESS == aAction) {
				state->rcktCtrl.isMoving = true;
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
	}

	void updateRocket(State_::rcktCtrl_& rocket, float dt, Mat44f& model2worldRocket) {
		if (rocket.isMoving) {
			// Store previous position for direction calculation
			Vec3f previousPosition = rocket.position;

			rocket.time += dt;

			// Update velocity with smaller acceleration
			rocket.velocity.y += rocket.acceleration * rocket.time * 0.01;
			if (rocket.time > 5) {
				rocket.velocity.x += rocket.acceleration * (rocket.time-5) * 0.5;
				rocket.velocity.z = -0.01f * dt;
			}
			

			// Update position for curved path with smaller values
			rocket.position.y += rocket.velocity.y * dt;   
			rocket.position.x += rocket.velocity.x * 0.2 * dt;
			rocket.position.z += rocket.velocity.z;
			

			// Calculate the direction of motion
			Vec3f direction = {
				rocket.position.x - previousPosition.x,
				rocket.position.y - previousPosition.y,
				rocket.position.z - previousPosition.z
			};

			// Normalize the direction vector to prevent errors when calculating the rotation
			if (length(direction) > 0.001f) {
				direction = normalize(direction);
			}

			rocket.pitch = atan2(direction.y, sqrt(direction.x * direction.x + direction.z * direction.z)) - M_PI/2;

			Mat44f rotationMatrix = make_rotation_z(rocket.pitch);
			model2worldRocket = make_translation(rocket.position) * rotationMatrix;

			// Debug output for verification
			printf("Rocket Time: %f\n", rocket.time);
			printf("Position: (%f, %f, %f)\n", rocket.position.x, rocket.position.y, rocket.position.z);
			printf("Velocity: (%f, %f, %f)\n", rocket.velocity.x, rocket.velocity.y, rocket.velocity.z);
			printf("Rotation Angle (Pitch): %f radians\n", rocket.pitch);
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
