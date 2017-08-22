#include <iostream>
#include <chrono>
#include <thread>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <shader_set.hpp>

#include <scene.hpp>
#include <renderer.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>

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
	static int lastX =  globals::glfw_framebuffer_width / 2.0;
	static int lastY =  globals::glfw_framebuffer_height / 2.0;
	static float fov   = 90.0f;
	static const float minfov = 10.0f;
	static const float maxfov = 110.0f;


	static bool forward  = false;
	static bool backward = false;
	static bool left     = false;
	static bool right    = false;
	static bool shift    = false;
	static bool alt      = false;
}

namespace consts {
	static const float window_target_fps = 64.0f;
	static const float window_target_frametime = 1.0f / window_target_fps;
	static const char* window_title = "rendercat";
}

// ---------------------------------- helpers  ---------------------------------
void GLAPIENTRY gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei /*length*/, const GLchar *message, const void */*userParam*/)
{
	std::cerr << "[GL] ";
	switch (source)
	{
		case GL_DEBUG_SOURCE_API:             std::cerr << "API "; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cerr << "Window System "; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cerr << "Shader Compiler "; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cerr << "Third Party "; break;
		case GL_DEBUG_SOURCE_APPLICATION:     std::cerr << "Application "; break;
		case GL_DEBUG_SOURCE_OTHER:           std::cerr << "Other "; break;
	}

	switch (type)
	{
		case GL_DEBUG_TYPE_ERROR:               std::cerr << "error "; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cerr << "Deprecated Behaviour "; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cerr << "Undefined Behaviour "; break;
		case GL_DEBUG_TYPE_PORTABILITY:         std::cerr << "Portability "; break;
		case GL_DEBUG_TYPE_PERFORMANCE:         std::cerr << "Performance "; break;
		case GL_DEBUG_TYPE_MARKER:              std::cerr << "Marker "; break;
		case GL_DEBUG_TYPE_PUSH_GROUP:          std::cerr << "Push Group "; break;
		case GL_DEBUG_TYPE_POP_GROUP:           std::cerr << "Pop Group "; break;
		case GL_DEBUG_TYPE_OTHER:               std::cerr << "Other "; break;
	}

	switch (severity)
	{
		case GL_DEBUG_SEVERITY_HIGH:         std::cerr << "(high) "; break;
		case GL_DEBUG_SEVERITY_MEDIUM:       std::cerr << "(med)  "; break;
		case GL_DEBUG_SEVERITY_LOW:          std::cerr << "(low)  "; break;
		case GL_DEBUG_SEVERITY_NOTIFICATION: std::cerr << "(info) "; break;
	}

	std::cerr << "[" << id << "]:\t" << message << '\n';
}


// ------------------------------- declarations --------------------------------

static void gflw_mouse_motion_callback(GLFWwindow* window, double dx, double dy);
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
			glfwSetCursorDeltaCallback(window, gflw_mouse_motion_callback);
			glfwSetCharCallback(window, nullptr);
		} else {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			glfwSetCharCallback(window, ImGui_ImplGlfwGL3_CharCallback);
			glfwSetCursorDeltaCallback(window, nullptr);
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

void gflw_mouse_motion_callback(GLFWwindow* /*window*/, double dx, double dy)
{
	if(unlikely(input::mouse_init)) // this bool variable is initially set to true
	{
		input::lastX = dx;
		input::lastY = dy;
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


static void assert_gl_default_framebuffer_is_srgb()
{
	GLint param;
	glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_FRONT_LEFT, GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, &param);
	if(param != GL_SRGB)
		throw std::runtime_error("Default framebuffer is not SRGB!");

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
	glfwSetCursorDeltaCallback(window, gflw_mouse_motion_callback);
	glfwSetMouseButtonCallback(window, glfw_mouse_click_callback);
	glfwSetScrollCallback(window, glfw_scroll_callback);
	glfwSetFramebufferSizeCallback(window, glfw_framebuffer_resized_callback);
}

//------------------------------------------------------------------------------
int main() try
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#ifndef NDEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
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

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		throw std::runtime_error("Failed to initialize GLEW.");
	}

	//print_all_extensions();
	enable_gl_debug_callback();
	enable_gl_clip_control();
	assert_gl_default_framebuffer_is_srgb();

	{
		ImGui_ImplGlfwGL3_Init(window);
		ImGuiStyle& st = ImGui::GetStyle();

		st.WindowRounding = 3.0f;
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
