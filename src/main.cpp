#include <chrono>
#include <thread>
#include <fmt/format.h>
#include <sstream>
#include <ios>

#include <zcm/angle_and_trigonometry.hpp>

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
#include <rendercat/util/gl_screenshot.hpp>
#include <rendercat/util/gl_meta.hpp>

#include <imgui.hpp>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#define DOCTEST_CONFIG_IMPLEMENT
#define DOCTEST_CONFIG_WINDOWS_SEH
#define DOCTEST_CONFIG_COLORS_NONE
#include <doctest/doctest.h>

using namespace gl45core;

namespace consts {
	static const char* window_title = "rendercat";
	static bool        window_maximized = false;
	static bool        window_fullscreen = false;
	static int         window_monitor_id = 0;
}

namespace globals {

	static float glfw_target_fps = 60.0f;
	static bool glfw_framebuffer_resized = false;
	static int  glfw_framebuffer_width = 1280;
	static int  glfw_framebuffer_height = 720;

	static float delta_time = 1.0f / glfw_target_fps;
	static float last_frame_time = 0.0f;
}

namespace input {
	static bool captured = true;
	static bool mouse_just_captured;
	static float sensitivity = 0.02f;

	static float xoffset = 0.0f;
	static float yoffset = 0.0f;
	static float scroll_offset = 0.0f;


	static bool forward  = false;
	static bool backward = false;
	static bool left     = false;
	static bool right    = false;
	static bool key_z    = false;
	static bool shift    = false;
	static bool alt      = false;

	static bool screenshot_requested = false;
	static int  screenshot_timeout   = 0;
}

// ---------------------------------- helpers  ---------------------------------
void GLAPIENTRY gl_debug_callback(GLenum source,
                                  GLenum type,
                                  GLuint id,
                                  GLenum severity,
                                  GLsizei /*length*/,
                                  const GLchar *message,
                                  const void* /*userParam*/)
{

	std::stringstream ss;
	ss << "\n[GL] ";
	switch (source)
	{
		case GL_DEBUG_SOURCE_API:             ss << "OpenGL API "; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   ss << "Window System "; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER: ss << "Shader Compiler "; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:     ss << "Third Party "; break;
		case GL_DEBUG_SOURCE_APPLICATION:     ss << "Application "; break;
		case GL_DEBUG_SOURCE_OTHER:           ss << "Other "; break;
		default: break;
	}

	switch (type)
	{
		case GL_DEBUG_TYPE_ERROR:               ss << "Error "; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: ss << "Deprecated Behavior"; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  ss << "Undefined Behavior"; break;
		case GL_DEBUG_TYPE_PORTABILITY:         ss << "Portability "; break;
		case GL_DEBUG_TYPE_PERFORMANCE:         ss << "Performance "; break;
		case GL_DEBUG_TYPE_MARKER:              ss << "Marker "; break;
		case GL_DEBUG_TYPE_OTHER:               ss << "Other "; break;
		default: break;
	}

	switch (severity)
	{
		case GL_DEBUG_SEVERITY_HIGH:         ss << "(high) "; break;
		case GL_DEBUG_SEVERITY_MEDIUM:       ss << "(med)  "; break;
		case GL_DEBUG_SEVERITY_LOW:          ss << "(low)  "; break;
		case GL_DEBUG_SEVERITY_NOTIFICATION: ss << "(info) "; break;
		default: break;
	}

	ss << "[" << id << "]\n\t" << message << '\n';
	auto str = ss.str();
	std::fwrite(str.data(), sizeof(char), str.size(), stderr);
	std::fflush(stderr);
}

// resolves deleter functions ahead of time to avoid destruction order issues
static void resolve_gl_functions() {
	glbinding::Binding::DeleteFramebuffers.resolveAddress();
	glbinding::Binding::DeleteQueries.resolveAddress();
	glbinding::Binding::DeleteShader.resolveAddress();
	glbinding::Binding::DeleteProgram.resolveAddress();
	glbinding::Binding::DeleteBuffers.resolveAddress();
	glbinding::Binding::DeleteTextures.resolveAddress();
	glbinding::Binding::DeleteVertexArrays.resolveAddress();
}


// ------------------------------- declarations --------------------------------

static void glfw_mouse_motion_callback(GLFWwindow* window, double dx, double dy);
static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
static void glfw_scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
static void glfw_framebuffer_resized_callback(GLFWwindow* window, int width, int height);


static void rc_set_input_captured(GLFWwindow* window, bool value)
{
	input::captured = value;
	if(input::captured) {
		input::mouse_just_captured = true;
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		glfwSetCursorDeltaCallback(window, glfw_mouse_motion_callback);
		glfwSetCharCallback(window, nullptr);
		glfwSetMouseButtonCallback(window, nullptr);
		glfwSetScrollCallback(window, glfw_scroll_callback);
	} else {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		glfwSetCharCallback(window, ImGui_ImplGlfw_CharCallback);
		glfwSetCursorDeltaCallback(window, nullptr);
		glfwSetMouseButtonCallback(window, ImGui_ImplGlfw_MouseButtonCallback);
		glfwSetScrollCallback(window, ImGui_ImplGlfw_ScrollCallback);
	}
}


static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if(key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) {
		rc_set_input_captured(window, !input::captured);
	}

	// if not capturing input, just call GUI key callback
	if(!input::captured) {
		ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
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
	case GLFW_KEY_Z:
		input::key_z = (action != GLFW_RELEASE);
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

static void glfw_process_input(rc::Scene* s, float dt)
{
	float cameraSpeed = 3.0f * dt;

	if(input::shift)
		cameraSpeed *= 5.0f;
	else if(input::alt)
		cameraSpeed *= 0.5f;

	if (input::forward)
		s->main_camera.behavior.move_forward(s->main_camera.state, cameraSpeed);
	if (input::backward)
		s->main_camera.behavior.move_forward(s->main_camera.state, -cameraSpeed);
	if (input::left)
		s->main_camera.behavior.move_right(s->main_camera.state, -cameraSpeed);
	if (input::right)
		s->main_camera.behavior.move_right(s->main_camera.state, cameraSpeed);

	if(input::key_z) {
		s->main_camera.behavior.roll(s->main_camera.state, zcm::radians(input::xoffset));
	} else {
		s->main_camera.behavior.on_mouse_move(s->main_camera.state, input::xoffset, input::yoffset);
	}

	s->main_camera.behavior.zoom(s->main_camera.state, -input::scroll_offset);
	input::xoffset = 0.0f;
	input::yoffset = 0.0f;
	input::scroll_offset = 0.0f;
}

void glfw_mouse_motion_callback(GLFWwindow* /*window*/, double dx, double dy)
{
	if(unlikely(input::mouse_just_captured)) // this bool variable is initially set to true
	{
		input::mouse_just_captured = false;
		return;
	}

	input::xoffset += dx * input::sensitivity;
	input::yoffset += dy * input::sensitivity;
}

static void glfw_scroll_callback(GLFWwindow* /*window*/, double /*xoffset*/, double yoffset)
{
	input::scroll_offset += yoffset;
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
	if(!(rc::glmeta::extension_supported(gl::GLextension::GL_KHR_debug) || rc::glmeta::extension_supported(gl::GLextension::GL_ARB_debug_output)))
		return;
	//glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	//glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, 0, GL_FALSE);
	glDebugMessageCallback(gl_debug_callback, 0);
#endif
}


static void check_gl_default_framebuffer_is_srgb()
{
	GLint param;
	glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_FRONT_LEFT, GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, &param);
	if(param != GL_SRGB) {
		std::fputs("[renderer] default framebuffer is not SRGB! Possible driver bug, check colors manually.\n", stderr);
	}

}

static void check_required_extensions()
{
	const gl::GLextension exts[] = {gl::GLextension::GL_ARB_clip_control,
	                                gl::GLextension::GL_ARB_direct_state_access,
	                                gl::GLextension::GL_ARB_framebuffer_sRGB};

	for(unsigned i = 0; i < std::size(exts); ++i) {
		rc::glmeta::require_extension(exts[i]);
	}
}

static void init_glfw_callbacks(GLFWwindow* window)
{
	rc_set_input_captured(window, input::captured);
	glfwSetKeyCallback(window, glfw_key_callback); // required because we always need to toggle input capture
	glfwSetFramebufferSizeCallback(window, glfw_framebuffer_resized_callback);
}

static void process_screenshot()
{
	if(input::screenshot_requested && input::screenshot_timeout == 0) {
		fmt::print("taken screenshot\n");
		input::screenshot_requested = false;
		input::screenshot_timeout = 200;
		rc::util::gl_screenshot(globals::glfw_framebuffer_width,
		              globals::glfw_framebuffer_height,
		              "screenshot.png");
	}
	if(input::screenshot_timeout > 0) {
		input::screenshot_requested = false;
		--input::screenshot_timeout;
	}
}

void run_tests(int argc, char *argv[])
{
#ifndef DOCTEST_CONFIG_DISABLE
	doctest::Context context;
	context.applyCommandLine(argc, argv);
	int res = context.run();
	if (context.shouldExit())
		std::exit(res);
#endif
}

//------------------------------------------------------------------------------
int main(int argc, char *argv[]) try
{
	run_tests(argc, argv);
	std::ios_base::sync_with_stdio(false);
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
		throw std::runtime_error("Failed to create GLFW window. Perhaps requested OpenGL version is not supported?");
	}

	if (glfwRawMouseMotionSupported())
		glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

	glfwMakeContextCurrent(window);

	if (consts::window_maximized) {
		glfwMaximizeWindow(window);
		glfwGetWindowSize(window, &globals::glfw_framebuffer_width, &globals::glfw_framebuffer_height);
		fmt::print("maximized window size is {} x {}\n", globals::glfw_framebuffer_width, globals::glfw_framebuffer_height);
	} else if (consts::window_fullscreen) {
		int monitor_count = -1;
		auto monitors = glfwGetMonitors(&monitor_count);

		auto monitor = consts::window_monitor_id < monitor_count ? monitors[consts::window_monitor_id] : glfwGetPrimaryMonitor();
		if (monitor) {
			int mode_count = -1;
			auto modes = glfwGetVideoModes(monitor, &mode_count);
			if (mode_count > 0) {

				globals::glfw_framebuffer_width = modes[mode_count-1].width;
				globals::glfw_framebuffer_height = modes[mode_count-1].height;
				int rate = modes[mode_count-1].refreshRate;
				globals::glfw_target_fps = rate;
				glfwSetWindowMonitor(window, monitor, 0, 0,
				                     globals::glfw_framebuffer_width,
				                     globals::glfw_framebuffer_height,
				                     rate);
			}
		}
	}

	glbinding::Binding::initialize(glfwGetProcAddress, false);

	rc::glmeta::log_all_supported_extensions("logs/gl_extensions.log");
	check_required_extensions();
	check_gl_default_framebuffer_is_srgb();
	resolve_gl_functions();
	enable_gl_debug_callback();
	enable_gl_clip_control();

	{
		ImGui::CreateContext();
		ImGui_ImplGlfw_InitForOpenGL(window, false);
		ImGui_ImplOpenGL3_Init("#version 430 core");

		auto& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_IsSRGB;

		auto& st = ImGui::GetStyle();
		ImGui::StyleColorsDark(&st);
		st.Colors[ImGuiCol_WindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.75f);
		st.GrabRounding = 0.0f;
		st.WindowRounding = 0.0f;
		st.WindowBorderSize = 0.0f;
		st.ChildRounding = 0.0f;
		st.ScrollbarRounding = 0.0f;
	}

	rc::Scene scene;
	rc::Renderer renderer(&scene);
	renderer.resize(globals::glfw_framebuffer_width,
	                globals::glfw_framebuffer_height);
	glfwPollEvents();
	renderer.clear_screen();
	glfwSwapBuffers(window);
	init_glfw_callbacks(window);
	scene.init();


	globals::last_frame_time = glfwGetTime();

	while(!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		glfw_process_input(&scene, globals::delta_time);

		if(globals::glfw_framebuffer_resized) {
			globals::glfw_framebuffer_resized = false;
			renderer.resize(globals::glfw_framebuffer_width,
			                globals::glfw_framebuffer_height);
		}
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		scene.update();

		renderer.draw();
		renderer.draw_gui();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		process_screenshot();

		glfwSwapBuffers(window);
		auto end_time = glfwGetTime();
		globals::delta_time = end_time - globals::last_frame_time;
		globals::last_frame_time = end_time;
		auto st = std::max((1.0f / globals::glfw_target_fps) - globals::delta_time, 0.0f);
		std::this_thread::sleep_for(std::chrono::duration<float>(st));
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwTerminate();
	return 0;

} catch(const fmt::format_error& e) {
	glfwTerminate();
	fmt::print("fmt::format_error exception: {}\n", e.what());
	return -1;
} catch(const std::exception& e) {
	glfwTerminate();
	fmt::print(stderr, "exception in main(): {}\n", e.what());
	return -1;
}
