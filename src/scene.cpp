#include <scene.hpp>

#include <stb_image.h>
#include <iostream>




Scene::Scene()
{
	float vertices[] = {
	        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
	         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
	         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
	         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
	        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
	        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

	        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
	         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
	         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
	         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
	        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
	        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

	        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
	        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
	        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
	        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
	        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
	        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

	         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
	         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
	         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
	         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
	         0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
	         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

	        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
	         0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
	         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
	         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
	        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
	        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

	        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
	         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
	         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
	         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
	        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
	        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
	};





	GLuint VBO, VAO;
	glGenBuffers(1,&VBO);
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// add vertices
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), nullptr);
	glEnableVertexAttribArray(0);

	// add colors
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), (GLvoid*)(3*sizeof(float)));
	glEnableVertexAttribArray(1);


	this->vao = VAO;
	glBindVertexArray(0);

	float maxAnisotropy;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);



	stbi_set_flip_vertically_on_load(1);
	// load and generate the texture
	int width, height, nrChannels;
	unsigned char *data = stbi_load("assets/materials/container.jpg", &width, &height, &nrChannels, 0);
	if (data)
	{
		GLuint tex;
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		this->texture = tex;
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);
}

