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

#define M_PI 3.14159265358979323846


#if defined(_WIN32) // alternative: ”#if defined(_MSC_VER)”

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
			float SLOW_SPEED_MULT = 0.1f;
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

	void createSpaceVehicle(
		std::vector<float>& vertices,
		std::vector<unsigned int>& indices,
		float rocketScale = 0.5f,
		float offsetX = 0.0f,
		float offsetY = 1.0f,
		float offsetZ = 0.0f		
	);

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
	GLuint langersoTextureObjectId = load_texture_2d(LANGERSO_TEXTURE_ASSET_PATH.c_str());
	std::size_t langersoVertexCount = langersoMesh.positions.size();

    // Load the spaceship
    GLuint spaceshipVao, spaceshipVbo, spaceshipEbo;
    std::size_t spaceshipIndexCount;

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

        // Generate rocket geometry (now 11 floats per vertex!)
        std::vector<float> vertices;
        std::vector<unsigned int> indices;

        createSpaceVehicle(vertices, indices, 0.1f, 0.f, 0.01f, 0.f);
        spaceshipIndexCount = indices.size();

        // Generate & Bind VAO
        glGenVertexArrays(1, &spaceshipVao);
        glBindVertexArray(spaceshipVao);

        // Generate & Bind VBO
        glGenBuffers(1, &spaceshipVbo);
        glBindBuffer(GL_ARRAY_BUFFER, spaceshipVbo);
        glBufferData(GL_ARRAY_BUFFER,
            vertices.size() * sizeof(float),
            vertices.data(),
            GL_STATIC_DRAW);

        // Generate & Bind EBO
        glGenBuffers(1, &spaceshipEbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, spaceshipEbo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            indices.size() * sizeof(unsigned int),
            indices.data(),
            GL_STATIC_DRAW);

        //    Define attribute pointers for 11-float layout
        //    position(3), color(3), normal(3), texCoord(2).
        //    => stride = 11 * sizeof(float).
        GLsizei stride = 11 * sizeof(float);

        // layout(location=0) => position
        glVertexAttribPointer(
            0,                   // index
            3,                   // size (x,y,z)
            GL_FLOAT,            // type
            GL_FALSE,            // normalized?
            stride,              // byte stride
            (void*)(0)           // offset in the array
        );
        glEnableVertexAttribArray(0);

        // layout(location=1) => color
        glVertexAttribPointer(
            1,
            3,
            GL_FLOAT,
            GL_FALSE,
            stride,
            (void*)(3 * sizeof(float))
        );
        glEnableVertexAttribArray(1);

        // layout(location=2) => normal
        glVertexAttribPointer(
            2,
            3,
            GL_FLOAT,
            GL_FALSE,
            stride,
            (void*)(6 * sizeof(float))
        );
        glEnableVertexAttribArray(2);

        // layout(location=3) => texCoord
        glVertexAttribPointer(
            3,
            2,
            GL_FLOAT,
            GL_FALSE,
            stride,
            (void*)(9 * sizeof(float))
        );
        glEnableVertexAttribArray(3);

        // Unbind VAO
        glBindVertexArray(0);


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

		Mat44f model = make_translation({ 0.0f, 0.0f, 0.0f }); // Adjust position as needed
		Mat44f view = make_look_at(
			state.camControl.position,
			state.camControl.position + state.camControl.forward,
			state.camControl.up
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


		// Draw spaceship
		GLint modelLoc = glGetUniformLocation(prog.programId(), "model");
		GLint viewLoc = glGetUniformLocation(prog.programId(), "view");
		GLint projectionLoc = glGetUniformLocation(prog.programId(), "projection");


		// Pass matrices to shaders
		glUniformMatrix4fv(modelLoc, 1, GL_TRUE, model.v);
		glUniformMatrix4fv(viewLoc, 1, GL_TRUE, view.v);
		glUniformMatrix4fv(projectionLoc, 1, GL_TRUE, projection.v);


		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, langersoTextureObjectId);
		glUniform1i(5, 1);

		// Give min and dims of the mesh to align tex coords with mesh 
		glUniform2f(6, langersoMesh.mins.x, langersoMesh.mins.y);
		glUniform2f(7, langersoMesh.diffs.x, langersoMesh.diffs.y);


		// Draw scene
		glUniformMatrix4fv(0, 1, GL_TRUE, projCameraWorldLangerso.v);
		glBindVertexArray(langersoVao);
		glDrawArrays(GL_TRIANGLES, 0, langersoVertexCount);

		glBindVertexArray(spaceshipVao);
		glDrawElements(GL_TRIANGLES, spaceshipIndexCount, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);


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

			// Changed to right click toggles camera
			//// Space toggles camera
			//if (GLFW_KEY_SPACE == aKey && GLFW_PRESS == aAction)
			//{
			//	state->camControl.cameraActive = !state->camControl.cameraActive;

			//	if (state->camControl.cameraActive)
			//		glfwSetInputMode(aWindow, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
			//	else
			//		glfwSetInputMode(aWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			//}

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
#include <vector>
#include <cmath>


    inline void pushVertex(
        std::vector<float>& verts,
        float px, float py, float pz,      // position
        float cr, float cg, float cb,      // color
        float nx, float ny, float nz,      // normal
        float u, float v                  // texcoords
    )
    {
        verts.push_back(px);
        verts.push_back(py);
        verts.push_back(pz);

        verts.push_back(cr);
        verts.push_back(cg);
        verts.push_back(cb);

        verts.push_back(nx);
        verts.push_back(ny);
        verts.push_back(nz);

        verts.push_back(u);
        verts.push_back(v);
    }



    static void generateCube(
        std::vector<float>& vertices,
        std::vector<unsigned int>& indices
    )
    {
        float cR = 0.1f, cG = 0.1f, cB = 0.1f; // Very dark gray
        unsigned int startIndex = static_cast<unsigned int>(vertices.size() / 11);


        // ---------- BACK FACE -----------
        // -0.5, -0.5, -0.5 => normal (0,0,-1), uv(0,0)
        pushVertex(vertices, -0.5f, -0.5f, -0.5f, cR, cG, cB, 0.f, 0.f, -1.f, 0.f, 0.f);
        pushVertex(vertices, 0.5f, -0.5f, -0.5f, cR, cG, cB, 0.f, 0.f, -1.f, 1.f, 0.f);
        pushVertex(vertices, 0.5f, 0.5f, -0.5f, cR, cG, cB, 0.f, 0.f, -1.f, 1.f, 1.f);
        pushVertex(vertices, -0.5f, 0.5f, -0.5f, cR, cG, cB, 0.f, 0.f, -1.f, 0.f, 1.f);

        // ---------- FRONT FACE -----------
        pushVertex(vertices, -0.5f, -0.5f, 0.5f, cR, cG, cB, 0.f, 0.f, 1.f, 0.f, 0.f);
        pushVertex(vertices, 0.5f, -0.5f, 0.5f, cR, cG, cB, 0.f, 0.f, 1.f, 1.f, 0.f);
        pushVertex(vertices, 0.5f, 0.5f, 0.5f, cR, cG, cB, 0.f, 0.f, 1.f, 1.f, 1.f);
        pushVertex(vertices, -0.5f, 0.5f, 0.5f, cR, cG, cB, 0.f, 0.f, 1.f, 0.f, 1.f);

        // ---------- LEFT FACE -----------
        pushVertex(vertices, -0.5f, 0.5f, 0.5f, cR, cG, cB, -1.f, 0.f, 0.f, 1.f, 0.f);
        pushVertex(vertices, -0.5f, 0.5f, -0.5f, cR, cG, cB, -1.f, 0.f, 0.f, 1.f, 1.f);
        pushVertex(vertices, -0.5f, -0.5f, -0.5f, cR, cG, cB, -1.f, 0.f, 0.f, 0.f, 1.f);
        pushVertex(vertices, -0.5f, -0.5f, 0.5f, cR, cG, cB, -1.f, 0.f, 0.f, 0.f, 0.f);

        // ---------- RIGHT FACE -----------
        pushVertex(vertices, 0.5f, 0.5f, 0.5f, cR, cG, cB, 1.f, 0.f, 0.f, 1.f, 0.f);
        pushVertex(vertices, 0.5f, 0.5f, -0.5f, cR, cG, cB, 1.f, 0.f, 0.f, 1.f, 1.f);
        pushVertex(vertices, 0.5f, -0.5f, -0.5f, cR, cG, cB, 1.f, 0.f, 0.f, 0.f, 1.f);
        pushVertex(vertices, 0.5f, -0.5f, 0.5f, cR, cG, cB, 1.f, 0.f, 0.f, 0.f, 0.f);

        // ---------- BOTTOM FACE -----------
        pushVertex(vertices, -0.5f, -0.5f, -0.5f, cR, cG, cB, 0.f, -1.f, 0.f, 0.f, 1.f);
        pushVertex(vertices, 0.5f, -0.5f, -0.5f, cR, cG, cB, 0.f, -1.f, 0.f, 1.f, 1.f);
        pushVertex(vertices, 0.5f, -0.5f, 0.5f, cR, cG, cB, 0.f, -1.f, 0.f, 1.f, 0.f);
        pushVertex(vertices, -0.5f, -0.5f, 0.5f, cR, cG, cB, 0.f, -1.f, 0.f, 0.f, 0.f);

        // ---------- TOP FACE -----------
        pushVertex(vertices, -0.5f, 0.5f, -0.5f, cR, cG, cB, 0.f, 1.f, 0.f, 0.f, 1.f);
        pushVertex(vertices, 0.5f, 0.5f, -0.5f, cR, cG, cB, 0.f, 1.f, 0.f, 1.f, 1.f);
        pushVertex(vertices, 0.5f, 0.5f, 0.5f, cR, cG, cB, 0.f, 1.f, 0.f, 1.f, 0.f);
        pushVertex(vertices, -0.5f, 0.5f, 0.5f, cR, cG, cB, 0.f, 1.f, 0.f, 0.f, 0.f);

        for (int face = 0; face < 6; ++face)
        {
            unsigned int idx0 = startIndex + face * 4 + 0;
            unsigned int idx1 = startIndex + face * 4 + 1;
            unsigned int idx2 = startIndex + face * 4 + 2;
            unsigned int idx3 = startIndex + face * 4 + 3;

            // 1st triangle
            indices.push_back(idx0);
            indices.push_back(idx1);
            indices.push_back(idx2);

            // 2nd triangle
            indices.push_back(idx2);
            indices.push_back(idx3);
            indices.push_back(idx0);
        }
    }


    static void generateCylinder(
        float radius, float height, int segments,
        std::vector<float>& vertices,
        std::vector<unsigned int>& indices
    )
    {
        float cR = 0.3f, cG = 0.3f, cB = 0.3f; // Medium gray
        unsigned int startIndex = static_cast<unsigned int>(vertices.size() / 11);

        // ---- TOP CENTER VERTEX ----
        {
            // position(0,height,0), color(cR,cG,cB), normal(0,1,0), uv(0.5,0.5)
            pushVertex(vertices,
                0.f, height, 0.f,
                cR, cG, cB,
                0.f, 1.f, 0.f,
                0.5f, 0.5f);
        }

        // For top ring
        for (int i = 0; i <= segments; ++i)
        {
            float theta = 2.0f * float(M_PI) * float(i) / float(segments);
            float x = radius * cosf(theta);
            float z = radius * sinf(theta);

            float u = (cosf(theta) * 0.5f) + 0.5f;
            float v = (sinf(theta) * 0.5f) + 0.5f;

            // position(x,height,z), color(cR,cG,cB), normal(0,1,0), uv(u,v)
            pushVertex(vertices,
                x, height, z,
                cR, cG, cB,
                0.f, 1.f, 0.f,
                u, v);

            if (i < segments)
            {
                unsigned int centerIdx = startIndex; // top center
                unsigned int currIdx = startIndex + i + 1;
                unsigned int nextIdx = startIndex + i + 2;
                indices.push_back(centerIdx);
                indices.push_back(currIdx);
                indices.push_back(nextIdx);
            }
        }

        // ---- BOTTOM CENTER VERTEX ----
        unsigned int bottomCenterIndex = static_cast<unsigned int>(vertices.size() / 11);
        {
            pushVertex(vertices,
                0.f, 0.f, 0.f,
                cR, cG, cB,
                0.f, -1.f, 0.f,
                0.5f, 0.5f);
        }

        // For bottom ring
        for (int i = 0; i <= segments; ++i)
        {
            float theta = 2.0f * float(M_PI) * float(i) / float(segments);
            float x = radius * cosf(theta);
            float z = radius * sinf(theta);

            float u = (cosf(theta) * 0.5f) + 0.5f;
            float v = (sinf(theta) * 0.5f) + 0.5f;

            pushVertex(vertices,
                x, 0.f, z,
                cR, cG, cB,
                0.f, -1.f, 0.f,
                u, v);

            if (i < segments)
            {
                unsigned int centerIdx = bottomCenterIndex;
                unsigned int currIdx = bottomCenterIndex + i + 1;
                unsigned int nextIdx = bottomCenterIndex + i + 2;
                // reversed order to keep consistent facing
                indices.push_back(centerIdx);
                indices.push_back(nextIdx);
                indices.push_back(currIdx);
            }
        }

        // ---- SIDE / WALLS ----
        unsigned int sideStartIndex = static_cast<unsigned int>(vertices.size() / 11);

        for (int i = 0; i <= segments; ++i)
        {
            float theta = 2.0f * float(M_PI) * float(i) / float(segments);
            float x = radius * cosf(theta);
            float z = radius * sinf(theta);

            float len = sqrtf(x * x + z * z);
            float nx = (len > 0.f) ? x / len : 0.f;
            float nz = (len > 0.f) ? z / len : 0.f;

            float u = float(i) / float(segments);

            // bottom vertex
            pushVertex(vertices,
                x, 0.f, z,
                cR, cG, cB,
                nx, 0.f, nz,
                u, 0.f);

            // top vertex
            pushVertex(vertices,
                x, height, z,
                cR, cG, cB,
                nx, 0.f, nz,
                u, 1.f);

            if (i < segments)
            {
                unsigned int bottom1 = sideStartIndex + i * 2 + 0;
                unsigned int top1 = sideStartIndex + i * 2 + 1;
                unsigned int bottom2 = sideStartIndex + (i + 1) * 2 + 0;
                unsigned int top2 = sideStartIndex + (i + 1) * 2 + 1;

                indices.push_back(bottom1);
                indices.push_back(top1);
                indices.push_back(bottom2);

                indices.push_back(bottom2);
                indices.push_back(top1);
                indices.push_back(top2);
            }
        }
    }


    static void generateCone(
        float radius, float height, int segments,
        std::vector<float>& vertices,
        std::vector<unsigned int>& indices
    )
    {
        float cR = 0.7f, cG = 0.7f, cB = 0.7f; // Lighter gray
        unsigned int startIndex = static_cast<unsigned int>(vertices.size() / 11);

        // apex (tip)
        pushVertex(vertices,
            0.f, height, 0.f,
            cR, cG, cB,
            0.f, 1.f, 0.f,
            0.5f, 0.5f);

        // base center
        unsigned int baseCenterIndex = static_cast<unsigned int>(vertices.size() / 11);
        pushVertex(vertices,
            0.f, 0.f, 0.f,
            cR, cG, cB,
            0.f, -1.f, 0.f,
            0.5f, 0.5f);

        // ring around base
        for (int i = 0; i <= segments; ++i)
        {
            float theta = 2.f * float(M_PI) * float(i) / float(segments);
            float x = radius * cosf(theta);
            float z = radius * sinf(theta);

            float u = (cosf(theta) * 0.5f) + 0.5f;
            float v = (sinf(theta) * 0.5f) + 0.5f;

            // For the base ring, let’s store normal downward or just -y
            pushVertex(vertices,
                x, 0.f, z,
                cR, cG, cB,
                0.f, -1.f, 0.f,
                u, v);

            if (i < segments)
            {
                unsigned int cIdx = baseCenterIndex;
                unsigned int currIdx = baseCenterIndex + i + 1;
                unsigned int nextIdx = baseCenterIndex + i + 2;
                indices.push_back(cIdx);
                indices.push_back(nextIdx);
                indices.push_back(currIdx);
            }
        }

        // side
        unsigned int sideStartIndex = static_cast<unsigned int>(vertices.size() / 11);
        unsigned int apexIndex = startIndex;

        for (int i = 0; i <= segments; ++i)
        {
            float theta = 2.f * float(M_PI) * float(i) / float(segments);
            float x = radius * cosf(theta);
            float z = radius * sinf(theta);

            // approximate slant normal
            float nx = x;
            float ny = (radius / height);
            float nz = z;
            float len = sqrtf(nx * nx + ny * ny + nz * nz);
            if (len > 0.f) {
                nx /= len; ny /= len; nz /= len;
            }

            float u = float(i) / float(segments);

            // side ring
            pushVertex(vertices,
                x, 0.f, z,
                cR, cG, cB,
                nx, ny, nz,
                u, 1.f);
        }

        // side indices
        for (int i = 0; i < segments; ++i)
        {
            unsigned int currIdx = sideStartIndex + i;
            unsigned int nextIdx = sideStartIndex + i + 1;
            indices.push_back(apexIndex);
            indices.push_back(currIdx);
            indices.push_back(nextIdx);
        }
    }


    static void generateRightAngledTriPrism(
        float base,
        float height,
        float thickness,
        std::vector<float>& outVertices,
        std::vector<unsigned int>& outIndices
    )
    {
        float cR = 0.7f, cG = 0.7f, cB = 0.7f; // same lighter gray
        unsigned int startIndex = static_cast<unsigned int>(outVertices.size() / 11);

        // v0 top
        pushVertex(outVertices,
            0.f, 0.f, 0.f,
            cR, cG, cB,
            0.f, 1.f, 0.f,
            0.f, 0.f);

        // v1 top
        pushVertex(outVertices,
            base, 0.f, 0.f,
            cR, cG, cB,
            0.f, 1.f, 0.f,
            1.f, 0.f);

        // v2 top
        pushVertex(outVertices,
            0.f, height, 0.f,
            cR, cG, cB,
            0.f, 1.f, 0.f,
            0.f, 1.f);

        // v3 bottom
        pushVertex(outVertices,
            0.f, 0.f, -thickness,
            cR, cG, cB,
            0.f, -1.f, 0.f,
            0.f, 0.f);

        // v4 bottom
        pushVertex(outVertices,
            base, 0.f, -thickness,
            cR, cG, cB,
            0.f, -1.f, 0.f,
            1.f, 0.f);

        // v5 bottom
        pushVertex(outVertices,
            0.f, height, -thickness,
            cR, cG, cB,
            0.f, -1.f, 0.f,
            0.f, 1.f);

        // indices
        std::vector<unsigned int> localIndices = {
            // top tri (v0,v1,v2)
            0,1,2,

            // bottom tri (v3,v5,v4)
            3,5,4,

            // side1 (v0,v1,v4,v3)
            0,1,4,
            0,4,3,

            // side2 (v0,v2,v5,v3)
            0,2,5,
            0,5,3,

            // side3 (v1,v2,v5,v4)
            1,2,5,
            1,5,4
        };

        for (auto idx : localIndices)
        {
            outIndices.push_back(startIndex + idx);
        }
    }


    void createSpaceVehicle(
        std::vector<float>& vertices,
        std::vector<unsigned int>& indices,
        float rocketScale,
        float offsetX,
        float offsetY,
        float offsetZ
    )
    {
        // REMEMBER STARTING VERTEX COUNT
        size_t startVertexCount = vertices.size() / 11;  // now /11

        // GENERATE ROCKET BODY (cylinder)
        {
            float bodyRadius = 0.2f;
            float bodyHeight = 1.0f;
            generateCylinder(bodyRadius, bodyHeight, 24, vertices, indices);
        }

        // GENERATE NOSE (cone) & shift it above the body
        {
            unsigned int vertexCountBefore = static_cast<unsigned int>(vertices.size() / 11);

            float noseRadius = 0.2f;
            float noseHeight = 0.4f;
            generateCone(noseRadius, noseHeight, 24, vertices, indices);

            // shift the cone up by bodyHeight
            unsigned int vertexCountAfter = static_cast<unsigned int>(vertices.size() / 11);
            for (unsigned int v = vertexCountBefore; v < vertexCountAfter; ++v)
            {
                // each vertex is 11 floats
                size_t base = v * 11;
                // "y" = vertices[base+1]
                vertices[base + 1] += 1.0f; // move the cone up by 1.0
            }
        }

        // ADD A SINGLE LARGE CUBE BOOSTER
        {
            std::vector<float> boosterCubeVerts;
            std::vector<unsigned int> boosterCubeIndices;
            generateCube(boosterCubeVerts, boosterCubeIndices);

            float boosterSize = 0.2f;
            float boosterOffset = 0.0f;

            auto addOneBigBooster = [&](float angleRadians)
            {
                unsigned int startLocal = static_cast<unsigned int>(vertices.size() / 11);

                // copy geometry
                for (size_t i = 0; i < boosterCubeVerts.size(); i += 11)
                {
                    float x = boosterCubeVerts[i + 0];
                    float y = boosterCubeVerts[i + 1];
                    float z = boosterCubeVerts[i + 2];

                    float r = boosterCubeVerts[i + 3];
                    float g = boosterCubeVerts[i + 4];
                    float b = boosterCubeVerts[i + 5];

                    float nx = boosterCubeVerts[i + 6];
                    float ny = boosterCubeVerts[i + 7];
                    float nz = boosterCubeVerts[i + 8];

                    float u = boosterCubeVerts[i + 9];
                    float v = boosterCubeVerts[i + 10];

                    // scale in X/Z
                    x *= boosterSize;
                    z *= boosterSize;
                    // maybe scale y differently
                    y *= boosterSize * 0.2f;

                    // offset from rocket center?
                    float px = boosterOffset * cosf(angleRadians);
                    float pz = boosterOffset * sinf(angleRadians);
                    x += px;
                    z += pz;

                    // push to main array
                    pushVertex(vertices,
                        x, y, z, r, g, b, nx, ny, nz, u, v);
                }

                // fix up indices
                for (auto idx : boosterCubeIndices)
                {
                    indices.push_back(startLocal + idx);
                }
            };

            addOneBigBooster(0.0f);
        }

        // ADD PRIMARY TRIANGULAR FINS (4)
        {
            std::vector<float> triVerts;
            std::vector<unsigned int> triIndices;
            generateRightAngledTriPrism(0.3f, 0.2f, 0.02f, triVerts, triIndices);

            auto addTriFin = [&](float angleRadians, float baseY)
            {
                unsigned int startLocal = static_cast<unsigned int>(vertices.size() / 11);

                for (size_t i = 0; i < triVerts.size(); i += 11)
                {
                    float x = triVerts[i + 0];
                    float y = triVerts[i + 1];
                    float z = triVerts[i + 2];

                    float r = triVerts[i + 3];
                    float g = triVerts[i + 4];
                    float b = triVerts[i + 5];

                    float nx = triVerts[i + 6];
                    float ny = triVerts[i + 7];
                    float nz = triVerts[i + 8];

                    float u = triVerts[i + 9];
                    float v = triVerts[i + 10];

                    // shift upward
                    y += baseY;

                    // rotate around Y
                    float xRot = x * cosf(angleRadians) - z * sinf(angleRadians);
                    float zRot = x * sinf(angleRadians) + z * cosf(angleRadians);
                    x = xRot; z = zRot;

                    // outward from rocket
                    float rocketRadius = 0.2f;
                    x += rocketRadius * cosf(angleRadians);
                    z += rocketRadius * sinf(angleRadians);

                    // push
                    pushVertex(vertices,
                        x, y, z, r, g, b, nx, ny, nz, u, v);
                }

                for (auto idx : triIndices)
                {
                    indices.push_back(startLocal + idx);
                }
            };

            float primaryFinY = 0.f;
            for (int i = 0; i < 4; ++i)
            {
                float angle = float(i) * (2.f * float(M_PI) / 4.f);
                addTriFin(angle, primaryFinY);
            }
        }

        // ADD 2 SMALLER TRIANGULAR FINS (HIGHER ON THE ROCKET)
        {
            std::vector<float> triVerts;
            std::vector<unsigned int> triIndices;
            generateRightAngledTriPrism(0.2f, 0.15f, 0.01f, triVerts, triIndices);

            auto addSmallFin = [&](float angleRadians, float baseY)
            {
                unsigned int startLocal = static_cast<unsigned int>(vertices.size() / 11);

                for (size_t i = 0; i < triVerts.size(); i += 11)
                {
                    float x = triVerts[i + 0];
                    float y = triVerts[i + 1];
                    float z = triVerts[i + 2];

                    float r = triVerts[i + 3];
                    float g = triVerts[i + 4];
                    float b = triVerts[i + 5];

                    float nx = triVerts[i + 6];
                    float ny = triVerts[i + 7];
                    float nz = triVerts[i + 8];

                    float u = triVerts[i + 9];
                    float v = triVerts[i + 10];

                    y += baseY;

                    // rotate around Y
                    float xRot = x * cosf(angleRadians) - z * sinf(angleRadians);
                    float zRot = x * sinf(angleRadians) + z * cosf(angleRadians);
                    x = xRot; z = zRot;

                    float rocketRadius = 0.2f;
                    x += rocketRadius * cosf(angleRadians);
                    z += rocketRadius * sinf(angleRadians);

                    pushVertex(vertices,
                        x, y, z, r, g, b, nx, ny, nz, u, v);
                }

                for (auto idx : triIndices)
                {
                    indices.push_back(startLocal + idx);
                }
            };

            float smallerFinY = 0.7f;
            addSmallFin(0.f, smallerFinY); // front
            addSmallFin(M_PI, smallerFinY); // back
        }

        {
            size_t newVertexCount = vertices.size() / 11;
            for (size_t v = startVertexCount; v < newVertexCount; ++v)
            {
                size_t base = v * 11;
                // Scale
                vertices[base + 0] *= rocketScale;
                vertices[base + 1] *= rocketScale;
                vertices[base + 2] *= rocketScale;

                // Then offset
                vertices[base + 0] += offsetX;
                vertices[base + 1] += offsetY;
                vertices[base + 2] += offsetZ;
            }
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
