#include <iostream>
#include <iomanip>
#include <cmath>
#include <chrono>
#include <thread>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <shader_set.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <scene.hpp>
#include <renderer.hpp>







// ---------------------------------- globals ----------------------------------
namespace globals {

	static bool glfw_framebuffer_resized = false;
	static int  glfw_framebuffer_width = 1280;
	static int  glfw_framebuffer_height = 720;

	static float delta_time = 0.0f;
	static float last_frame_time = 0.0f;


	static bool mouse_captured = true;
	static bool mouse_button1 = false;
	static bool mouse_button2 = false;

	static bool mouse_first = true;
	static float yaw   = -90.0f;
	static float pitch =  0.0f;
	static float lastX =  glfw_framebuffer_width / 2.0;
	static float lastY =  glfw_framebuffer_height / 2.0;
	static float fov  = 60.0f;


	static bool forward = false;
	static bool backward = false;
	static bool left = false;
	static bool right = false;

	static bool mod_shift = false;
	static bool mod_alt = false;
	static bool mod_ctrl = false;

}

// --------------------------------- constants ---------------------------------
namespace consts {
	static const float window_target_fps = 64.0f;
	static const float window_target_frametime = 1.0f / window_target_fps;
	static const char* window_title = "glrun";
}

// ---------------------------------- helpers  ---------------------------------
void GLAPIENTRY gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
	std::cerr << "[GL] ";


	switch (source)
	{
		case GL_DEBUG_SOURCE_API:             std::cerr << "src: API  "; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cerr << "src: Window System  "; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cerr << "src: Shader Compiler  "; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cerr << "src: Third Party  "; break;
		case GL_DEBUG_SOURCE_APPLICATION:     std::cerr << "src: Application  "; break;
		case GL_DEBUG_SOURCE_OTHER:           std::cerr << "src: Other  "; break;
	}

	switch (type)
	{
		case GL_DEBUG_TYPE_ERROR:               std::cerr << "type: Error  "; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cerr << "type: Deprecated Behaviour  "; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cerr << "type: Undefined Behaviour  "; break;
		case GL_DEBUG_TYPE_PORTABILITY:         std::cerr << "type: Portability  "; break;
		case GL_DEBUG_TYPE_PERFORMANCE:         std::cerr << "type: Performance  "; break;
		case GL_DEBUG_TYPE_MARKER:              std::cerr << "type: Marker  "; break;
		case GL_DEBUG_TYPE_PUSH_GROUP:          std::cerr << "type: Push Group  "; break;
		case GL_DEBUG_TYPE_POP_GROUP:           std::cerr << "type: Pop Group  "; break;
		case GL_DEBUG_TYPE_OTHER:               std::cerr << "type: Other  "; break;
	}

	switch (severity)
	{
		case GL_DEBUG_SEVERITY_HIGH:         std::cerr << "sev: high  "; break;
		case GL_DEBUG_SEVERITY_MEDIUM:       std::cerr << "sev: medium  "; break;
		case GL_DEBUG_SEVERITY_LOW:          std::cerr << "sev: low  "; break;
		case GL_DEBUG_SEVERITY_NOTIFICATION: std::cerr << "sev: notification  "; break;
	}

	std::cerr << "msg (" << id << "):\t" << message << '\n';
}


// ------------------------------- declarations --------------------------------

static void gflw_mouse_motion_callback(GLFWwindow* window, double xpos, double ypos);
static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
static void glfw_mouse_click_callback(GLFWwindow* window, int button, int action, int mods);
static void glfw_focus_callback(GLFWwindow* window, int focused);
static void glfw_scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
static void glfw_framebuffer_resized_callback(GLFWwindow*, int width, int height);



static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	switch (key) {
	case GLFW_KEY_W:
		globals::forward = (action != GLFW_RELEASE);
		break;
	case GLFW_KEY_A:
		globals::left = (action != GLFW_RELEASE);
		break;
	case GLFW_KEY_S:
		globals::backward = (action != GLFW_RELEASE);
		break;
	case GLFW_KEY_D:
		globals::right = (action != GLFW_RELEASE);
		break;


	case GLFW_KEY_ESCAPE:
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		glfwSetCursorPosCallback(window, nullptr);
		globals::mouse_captured = false;
		break;
	default:
		break;
	}

	if(mods & GLFW_MOD_SHIFT) {
		globals::mod_shift = (action != GLFW_RELEASE);
	}
	if(mods & GLFW_MOD_ALT) {
		globals::mod_alt = (action != GLFW_RELEASE);
	}
	if(mods & GLFW_MOD_CONTROL) {
		globals::mod_ctrl = (action != GLFW_RELEASE);
	}
}

static void glfw_mouse_click_callback(GLFWwindow* window, int button, int action, int mods)
{
	if(!globals::mouse_captured && action == GLFW_PRESS)
	{
		globals::mouse_captured = true;
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		glfwSetCursorPosCallback(window, gflw_mouse_motion_callback);
	}

	if(button = GLFW_MOUSE_BUTTON_1) {
		if(action != GLFW_RELEASE) {
			globals::mouse_button1 = true;
		} else {
			globals::mouse_button1 = false;
		}
	}
	if(button = GLFW_MOUSE_BUTTON_2) {
		if(action != GLFW_RELEASE) {
			globals::mouse_button2 = true;
		} else {
			globals::mouse_button2 = false;
		}
	}
}


static void glfw_process_input(Scene* s)
{
	float cameraSpeed = 2.5f * globals::delta_time;

	if(globals::mod_shift)
		cameraSpeed *= 5.0f;

	if (globals::forward)
		s->main_camera.pos += cameraSpeed * s->main_camera.front;
	if (globals::backward)
		s->main_camera.pos -= cameraSpeed * s->main_camera.front;
	if (globals::left)
		s->main_camera.pos -= glm::normalize(glm::cross(s->main_camera.front, s->main_camera.up)) * cameraSpeed;
	if (globals::right)
		s->main_camera.pos += glm::normalize(glm::cross(s->main_camera.front, s->main_camera.up)) * cameraSpeed;


	glm::vec3 front;
	front.x = cos(glm::radians(globals::yaw)) * cos(glm::radians(globals::pitch));
	front.y = sin(glm::radians(globals::pitch));
	front.z = sin(glm::radians(globals::yaw)) * cos(glm::radians(globals::pitch));
	s->main_camera.front = glm::normalize(front);
	s->main_camera.fov = globals::fov;
}


void gflw_mouse_motion_callback(GLFWwindow* window, double xpos, double ypos)
{
	if(globals::mouse_first) // this bool variable is initially set to true
	{
		globals::lastX = xpos;
		globals::lastY = ypos;
		globals::mouse_first = false;
	}

	float xoffset = xpos - globals::lastX;
	float yoffset = globals::lastY - ypos; // reversed since y-coordinates go from bottom to top
	globals::lastX = xpos;
	globals::lastY = ypos;

	float sensitivity = 0.1f; // change this value to your liking
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	globals::yaw += xoffset;
	globals::pitch += yoffset;

	// make sure that when pitch is out of bounds, screen doesn't get flipped
	if (globals::pitch > 89.0f)
		globals::pitch = 89.0f;
	if (globals::pitch < -89.0f)
		globals::pitch = -89.0f;
}


static void glfw_focus_callback(GLFWwindow* window, int focused)
{
	std::cerr << "focus state: " <<focused;
	if(focused == GLFW_TRUE) {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		glfwSetCursorPosCallback(window, gflw_mouse_motion_callback);
	} else {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		glfwSetCursorPosCallback(window, nullptr);
	}
}


static void glfw_scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	if (globals::fov >= 1.0f && globals::fov <= 110.0f)
		globals::fov -= yoffset;
	if (globals::fov <= 1.0f)
		globals::fov = 1.0f;
	if (globals::fov >= 110.0f)
		globals::fov = 110.0f;
}


static void glfw_framebuffer_resized_callback(GLFWwindow*, int width, int height)
{
	globals::glfw_framebuffer_resized = true;
	globals::glfw_framebuffer_width = width;
	globals::glfw_framebuffer_height = height;
}


static void enable_gl_clip_control()
{
	GLint major, minor;
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);
	if ((major > 4 || (major == 4 && minor >= 5)) || glfwExtensionSupported("GL_ARB_clip_control"))
	{
		glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
	}
	else throw std::runtime_error("required opengl extension missing: GL_ARB_clip_control\n");

}


static void enable_gl_debug_callback()
{
#ifndef NDEBUG
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, 0, GL_FALSE);
	glDebugMessageCallback(gl_debug_callback, 0);
#endif
}



//------------------------------------------------------------------------------
int main() try
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_SAMPLES, 16);

	auto window = glfwCreateWindow(globals::glfw_framebuffer_width,
	                               globals::glfw_framebuffer_height,
	                               consts::window_title,
	                               nullptr,
	                               nullptr);

	if (window == nullptr) {
		throw std::runtime_error("Failed to create GLFW window.");
	}

	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, glfw_key_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(window, gflw_mouse_motion_callback);
	glfwSetMouseButtonCallback(window, glfw_mouse_click_callback);
	glfwSetWindowFocusCallback(window, glfw_focus_callback);
	glfwSetScrollCallback(window, glfw_scroll_callback);
	glfwSetFramebufferSizeCallback(window, glfw_framebuffer_resized_callback);

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		throw std::runtime_error("Failed to initialize GLEW.");
	}

	enable_gl_debug_callback();
	enable_gl_clip_control();

	Scene scene;
	Renderer renderer(&scene);
	renderer.resize(globals::glfw_framebuffer_width,
	                globals::glfw_framebuffer_height);


	globals::last_frame_time = glfwGetTime();

	while(!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		glfw_process_input(&scene);

		if(globals::glfw_framebuffer_resized) {
			globals::glfw_framebuffer_resized = false;
			std::cerr << "framebuffer resized to "
			          << globals::glfw_framebuffer_width
			          << " x "
			          << globals::glfw_framebuffer_height
			          << '\n';

			renderer.resize(globals::glfw_framebuffer_width,
			                globals::glfw_framebuffer_height);

		}


		renderer.draw(globals::last_frame_time);

		glfwSwapBuffers(window);

		auto end_time = glfwGetTime();
		globals::delta_time = end_time - globals::last_frame_time;
		globals::last_frame_time = end_time;
		auto st = std::max(consts::window_target_frametime - globals::delta_time, 0.0f);

		std::this_thread::sleep_for(std::chrono::duration<float>(st));
	}

	glfwTerminate();
	return 0;
} catch(const std::exception& e) {
	glfwTerminate();
	std::cerr << "exception in main():\n" << e.what() << std::endl;
	return -1;
}
