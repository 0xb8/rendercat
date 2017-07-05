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
	static int  glfw_framebuffer_width = 854;
	static int  glfw_framebuffer_height = 480;
}

// --------------------------------- constants ---------------------------------
namespace consts {
	static const float window_target_fps = 60.0f;
	static const float window_target_frametime = 1.0f / window_target_fps;
	static const char* window_title = "glrun";
}

// ------------------------------- helpers  --------------------------------
void GLAPIENTRY gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
	std::cerr << "[GL]\t" << message << '\n';
}



static void key_callback(GLFWwindow* window, int key, int, int action, int) noexcept
{
	if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
}


static void framebuffer_resized_callback(GLFWwindow*, int width, int height)
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
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

	auto window = glfwCreateWindow(globals::glfw_framebuffer_width,
	                               globals::glfw_framebuffer_height,
	                               consts::window_title,
	                               nullptr,
	                               nullptr);

	if (window == nullptr) {
		throw std::runtime_error("Failed to create GLFW window.");
	}

	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_callback);
	glfwSetFramebufferSizeCallback(window, framebuffer_resized_callback);

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		throw std::runtime_error("Failed to initialize GLEW.");
	}

	enable_gl_debug_callback();
	enable_gl_clip_control();

//	glViewport(0, 0, globals::glfw_framebuffer_width, globals::glfw_framebuffer_height);

//	ShaderSet shaders;
//	auto program = shaders.load_program({"triangle.vert", "triangle.frag"});



//	GLfloat square[] = {
//		 // pos             // color          // UV
//		 0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 2.0f, 2.0f, // top right
//		 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 2.0f, 0.0f, // bottom right
//		-0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, // bottom left
//		-0.5f,  0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 2.0f  // top left
//	};

//	GLuint indices[] = {
//		0, 1, 3,
//		1, 2, 3
//	};

//	GLuint VBO, VAO, EBO;
//	glGenBuffers(1,&VBO);
//	glGenVertexArrays(1, &VAO);
//	glBindVertexArray(VAO);

//	glBindBuffer(GL_ARRAY_BUFFER, VBO);
//	glBufferData(GL_ARRAY_BUFFER, sizeof(square), square, GL_STATIC_DRAW);

//	glGenBuffers(1, &EBO);
//	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
//	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

//	/* (location, sizeof(vec3), typeof(vec3.x), normalize, stride, offset) */
//	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), nullptr);
//	glEnableVertexAttribArray(0);

//	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (GLvoid*)(3*sizeof(float)));
//	glEnableVertexAttribArray(1);

//	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (GLvoid*)(6*sizeof(float)));
//	glEnableVertexAttribArray(2);

//	glBindVertexArray(0);



//	unsigned int texture;
//	glGenTextures(1, &texture);
//	glBindTexture(GL_TEXTURE_2D, texture);
//	// set the texture wrapping/filtering options (on the currently bound texture object)
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//	// load and generate the texture
//	int width, height, nrChannels;
//	unsigned char *data = stbi_load("assets/materials/container.jpg", &width, &height, &nrChannels, 0);
//	if (data)
//	{
//		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
//		glGenerateMipmap(GL_TEXTURE_2D);
//	}
//	else
//	{
//		std::cout << "Failed to load texture" << std::endl;
//	}
//	stbi_image_free(data);



	Scene scene;
	Renderer renderer(&scene);
	renderer.resize(globals::glfw_framebuffer_width,
	                globals::glfw_framebuffer_height);


	auto last_ft = glfwGetTime();
	//auto time_u = glGetUniformLocation(*program, "time");

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// main loop
	while(!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		if(globals::glfw_framebuffer_resized) {
			globals::glfw_framebuffer_resized = false;
			std::cerr << "framebuffer resized to "
			          << globals::glfw_framebuffer_width
			          << " x "
			          << globals::glfw_framebuffer_height
			          << '\n';

//			glViewport(0, 0,
//			           globals::glfw_framebuffer_width,
//			           globals::glfw_framebuffer_height);


			renderer.resize(globals::glfw_framebuffer_width,
			                globals::glfw_framebuffer_height);

		}


		renderer.draw(last_ft);


//		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
//		glClear(GL_COLOR_BUFFER_BIT);


//		if(program)
//			glUseProgram(*program);
//		else continue;



//		glActiveTexture(GL_TEXTURE0);
//		glBindTexture(GL_TEXTURE_2D, texture);
//		glUniform1f(time_u, last_ft);
//		glBindVertexArray(VAO);
//		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
//		glBindVertexArray(0);

		glfwSwapBuffers(window);

		auto end_time = glfwGetTime();
		auto dt = end_time - last_ft;
		last_ft = end_time;
		auto st = consts::window_target_frametime - dt;

		std::this_thread::sleep_for(std::chrono::duration<float>(st));
		//std::cout << "\rft: " << std::fixed << std::right << std::setprecision(2) << dt*1000 << "ms / "  << 1.0 / dt << "fps          ";
		//std::cout.flush();
	}

	// cleanup and exit
	glfwTerminate();
	return 0;
} catch(const std::exception& e) {
	glfwTerminate();
	std::cerr << "exception in main():\n" << e.what() << std::endl;
	return -1;
}



