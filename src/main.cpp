#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>

#include <glbinding/gl45core/boolean.h>
#include <glbinding/gl45core/types.h>
#include <glbinding/gl45core/enum.h>
#include <glbinding/gl45core/functions.h>
#include <glbinding/Binding.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <rendercat/shader_set.hpp>
#include <rendercat/scene.hpp>
#include <rendercat/renderer.hpp>
#include <rendercat/gl_screenshot.hpp>

#include <imgui.hpp>
#include <imgui_impl_glfw.h>

using namespace gl45core;

namespace globals {

	static bool glfw_framebuffer_resized = false;
	static int  glfw_framebuffer_width = 1280;
	static int  glfw_framebuffer_height = 720;

	static float delta_time = 0.0f;
	static float last_frame_time = 0.0f;
}

namespace input {
	static bool captured = true;
	static bool mouse_init = true;
	static float sensitivity = 0.02f;

	static float xoffset = 0.0f;
	static float yoffset = 0.0f;
	static float fov   = 90.0f;
	static const float minfov = 10.0f;
	static const float maxfov = 110.0f;


	static bool forward  = false;
	static bool backward = false;
	static bool left     = false;
	static bool right    = false;
	static bool shift    = false;
	static bool alt      = false;

	static bool screenshot_requested = false;
	static int  screenshot_timeout   = 0;


}

namespace consts {
	static const float window_target_fps = 64.0f;
	static const float window_target_frametime = 1.0f / window_target_fps;
	static const char* window_title = "rendercat";
}

// ---------------------------------- helpers  ---------------------------------
void GLAPIENTRY gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei /*length*/, const GLchar *message, const void */*userParam*/)
{

	std::stringstream ss;
	ss << "[GL] ";
	switch (source)
	{
		case GL_DEBUG_SOURCE_API:             ss<< "API "; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   ss << "Window System "; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER: ss << "Shader Compiler "; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:     ss << "Third Party "; break;
		case GL_DEBUG_SOURCE_APPLICATION:     ss << "Application "; break;
		case GL_DEBUG_SOURCE_OTHER:           ss << "Other "; break;
		default: break;
	}

	switch (type)
	{
		case GL_DEBUG_TYPE_ERROR:               ss << "error "; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: ss << "Deprecated Behaviour "; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  ss << "Undefined Behaviour "; break;
		case GL_DEBUG_TYPE_PORTABILITY:         ss << "Portability "; break;
		case GL_DEBUG_TYPE_PERFORMANCE:         ss << "Performance "; break;
		case GL_DEBUG_TYPE_MARKER:              ss << "Marker "; break;
		case GL_DEBUG_TYPE_PUSH_GROUP:          ss << "Push Group "; break;
		case GL_DEBUG_TYPE_POP_GROUP:           ss << "Pop Group "; break;
		case GL_DEBUG_TYPE_OTHER:               ss << "Other "; break;
		default: break;
	}

	switch (severity)
	{
		case GL_DEBUG_SEVERITY_HIGH:         ss << "(high) "; break;
		case GL_DEBUG_SEVERITY_MEDIUM:       ss << "(med)  "; break;
		case GL_DEBUG_SEVERITY_LOW:          ss << "(low)  "; break;
		case GL_DEBUG_SEVERITY_NOTIFICATION: ss<< "(info) "; break;
		default: break;
	}

	ss << "[" << id << "]:\t" << message << '\n';
	std::cerr << ss.rdbuf();
}


// ------------------------------- declarations --------------------------------

static void glfw_mouse_motion_callback(GLFWwindow* window, double dx, double dy);
static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
static void glfw_mouse_click_callback(GLFWwindow* window, int button, int action, int mods);
static void glfw_scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
static void glfw_framebuffer_resized_callback(GLFWwindow* window, int width, int height);


static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if(key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) {
		input::captured = !input::captured;
		if(input::captured) {
			input::mouse_init = true;
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			glfwSetCursorDeltaCallback(window, glfw_mouse_motion_callback);
			glfwSetCursorPosCallback(window, nullptr);
			glfwSetCharCallback(window, nullptr);
		} else {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			glfwSetCharCallback(window, ImGui_ImplGlfwGL3_CharCallback);
			glfwSetCursorDeltaCallback(window, nullptr);
			glfwSetCursorPosCallback(window, ImGui_ImplGlfwGL3_CursorPosCallback);
		}
	}

	if(!input::captured) {
		ImGui_ImplGlfwGL3_KeyCallback(window, key, scancode, action, mods);
		return;
	}

	switch (key) {
	case GLFW_KEY_W:
		input::forward = (action != GLFW_RELEASE);
		if(mods & GLFW_MOD_CONTROL)
			glfwSetWindowShouldClose(window,true);
		break;
	case GLFW_KEY_A:
		input::left = (action != GLFW_RELEASE);
		break;
	case GLFW_KEY_S:
		input::backward = (action != GLFW_RELEASE);
		break;
	case GLFW_KEY_D:
		input::right = (action != GLFW_RELEASE);
		break;
	case GLFW_KEY_LEFT_SHIFT:
		input::shift = (action != GLFW_RELEASE);
		break;
	case GLFW_KEY_LEFT_ALT:
		input::alt = (action != GLFW_RELEASE);
		break;
	case GLFW_KEY_F10:
		input::screenshot_requested = true;
		break;
	default:
		break;
	}
}

static void glfw_mouse_click_callback(GLFWwindow* window, int button, int action, int mods)
{
	if(!input::captured) {
		if(action == GLFW_PRESS && (mods & GLFW_MOD_SHIFT)) {

			return;
		}
		ImGui_ImplGlfwGL3_MouseButtonCallback(window, button, action, mods);
		return;
	}

	if(button == GLFW_MOUSE_BUTTON_3 && action == GLFW_PRESS) {
		input::fov = 90.0f;
	}
}

static void glfw_process_input(Scene* s)
{
	float cameraSpeed = 0.04;

	if(input::shift)
		cameraSpeed *= 5.0f;
	else if(input::alt)
		cameraSpeed *= 0.5f;

	if (input::forward)
		s->main_camera.forward(cameraSpeed);
	if (input::backward)
		s->main_camera.backward(cameraSpeed);
	if (input::left)
		s->main_camera.left(cameraSpeed);
	if (input::right)
		s->main_camera.right(cameraSpeed);

	s->main_camera.aim(input::xoffset, input::yoffset);
	s->main_camera.zoom(input::fov);
	input::xoffset = 0.0f;
	input::yoffset = 0.0f;
}

void glfw_mouse_motion_callback(GLFWwindow* /*window*/, double dx, double dy)
{
	if(unlikely(input::mouse_init)) // this bool variable is initially set to true
	{
		input::mouse_init = false;
		return;
	}

	input::xoffset += dx * input::sensitivity;
	input::yoffset += -dy * input::sensitivity;
}

static void glfw_scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	if(!input::captured) {
		ImGui_ImplGlfwGL3_ScrollCallback(window, xoffset, yoffset);
		return;
	}

	if (input::fov >= input::minfov && input::fov <= input::maxfov)
		input::fov -= yoffset;
	input::fov = glm::clamp(input::fov, input::minfov, input::maxfov);
}

static void glfw_framebuffer_resized_callback(GLFWwindow* /*window*/, int width, int height)
{
	globals::glfw_framebuffer_resized = true;
	globals::glfw_framebuffer_width = width;
	globals::glfw_framebuffer_height = height;
}

static void enable_gl_clip_control()
{
	glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
}

static void enable_gl_debug_callback()
{
#ifndef NDEBUG
	if(!glfwExtensionSupported("GL_KHR_debug") && !glfwExtensionSupported("GL_ARB_debug_output"))
		return;
	//glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, 0, GL_FALSE);
	glDebugMessageCallback(gl_debug_callback, 0);
#endif
}


static void check_gl_default_framebuffer_is_srgb()
{
	GLint param;
	glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_FRONT_LEFT, GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, &param);
	if(param != GL_SRGB) {
		std::cerr << "[renderer] default framebuffer is not SRGB! Possible driver bug, check colors manually.\n";
	}

}

static void check_required_extensions()
{
	const char* extensions[] = {"GL_ARB_direct_state_access",
	                            "GL_ARB_clip_control"};

	for(unsigned i = 0; i < std::size(extensions); ++i) {
		if(!glfwExtensionSupported(extensions[i])) {
			std::string err{"required opengl extension missing: "};
			err.append(extensions[i]);
			throw std::runtime_error(err);
		}
	}
}

static void print_all_extensions()
{
	int n;
	glGetIntegerv(GL_NUM_EXTENSIONS, &n);
	std::cerr << n << " supported extensions:\n";
	for(int i = 0; i <n; ++i) {
		std::cerr << glGetStringi(GL_EXTENSIONS, i) << '\n';
	}
}

static void init_glfw_callbacks(GLFWwindow* window)
{
	glfwSetKeyCallback(window, glfw_key_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorDeltaCallback(window, glfw_mouse_motion_callback);
	glfwSetMouseButtonCallback(window, glfw_mouse_click_callback);
	glfwSetScrollCallback(window, glfw_scroll_callback);
	glfwSetFramebufferSizeCallback(window, glfw_framebuffer_resized_callback);
}

static void process_screenshot()
{
	if(input::screenshot_requested && input::screenshot_timeout == 0) {
		input::screenshot_requested = false;
		input::screenshot_timeout = 200;
		gl_screenshot(globals::glfw_framebuffer_width,
		              globals::glfw_framebuffer_height,
		              "screenshot.png");
	}
	if(input::screenshot_timeout > 0) {
		input::screenshot_requested = false;
		--input::screenshot_timeout;
	}
}

//------------------------------------------------------------------------------
int main() try
{
	std::ios::sync_with_stdio(false);
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#ifndef NDEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif
	glfwWindowHint(GLFW_RESIZABLE,    GLFW_TRUE);
	glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

	auto window = glfwCreateWindow(globals::glfw_framebuffer_width,
	                               globals::glfw_framebuffer_height,
	                               consts::window_title,
	                               nullptr,
	                               nullptr);

	if (window == nullptr) {
		throw std::runtime_error("Failed to create GLFW window.");
	}

	glfwMakeContextCurrent(window);

	glbinding::Binding::initialize(false);

	//print_all_extensions();
	check_required_extensions();
	check_gl_default_framebuffer_is_srgb();
	enable_gl_debug_callback();
	enable_gl_clip_control();

	{
		ImGui_ImplGlfwGL3_Init(window);
		ImGuiStyle& st = ImGui::GetStyle();

		st.GrabRounding = 0.0f;
		st.WindowRounding = 0.0f;
		st.ChildWindowRounding = 0.0f;
		st.ScrollbarRounding = 0.0f;
	}

	Scene scene;
	Renderer renderer(&scene);
	renderer.resize(globals::glfw_framebuffer_width,
	                globals::glfw_framebuffer_height);

	init_glfw_callbacks(window);

	globals::last_frame_time = glfwGetTime();

	while(!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		glfw_process_input(&scene);
		ImGui_ImplGlfwGL3_NewFrame();

		scene.update();

		if(globals::glfw_framebuffer_resized) {
			globals::glfw_framebuffer_resized = false;
			renderer.resize(globals::glfw_framebuffer_width,
			                globals::glfw_framebuffer_height);
		}
		renderer.draw();
		renderer.draw_gui();
		ImGui::Render();
		process_screenshot();

		glfwSwapBuffers(window);
		auto end_time = glfwGetTime();
		globals::delta_time = end_time - globals::last_frame_time;
		globals::last_frame_time = end_time;
		auto st = std::max(consts::window_target_frametime - globals::delta_time, 0.0f);
		std::this_thread::sleep_for(std::chrono::duration<float>(st));
	}

	ImGui_ImplGlfwGL3_Shutdown();
	glfwTerminate();
	return 0;
} catch(const std::exception& e) {
	glfwTerminate();
	std::cerr << "exception in main():\n" << e.what() << std::endl;
	return -1;
}
