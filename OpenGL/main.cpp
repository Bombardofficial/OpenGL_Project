#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif
#include <iostream>
#include "glad.h"
#include <GLFW/glfw3.h>
#include <KHR/khrplatform.h>
#include "shaderClass.h"
#include "Texture.h"
#include "VAO.h"
#include "EBO.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Vertices coordinates
GLfloat vertices[] =
{ //     COORDINATES     /        COLORS      /   TexCoord  //
	-0.5f, -0.5f, 0.0f,     1.0f, 0.0f, 0.0f,	0.0f, 0.0f, // Lower left corner
	-0.5f,  0.5f, 0.0f,     0.0f, 1.0f, 0.0f,	0.0f, 1.0f, // Upper left corner
	 0.5f,  0.5f, 0.0f,     0.0f, 0.0f, 1.0f,	1.0f, 1.0f, // Upper right corner
	 0.5f, -0.5f, 0.0f,     1.0f, 1.0f, 1.0f,	1.0f, 0.0f  // Lower right corner
};

// Indices for vertices order
GLuint indices[] =
{
	0, 2, 1, // Upper triangle
	0, 3, 2 // Lower triangle
};

glm::vec3 spaceshipPosition = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 spaceshipScale = glm::vec3(0.2f, 0.2f, 0.2f); // Adjust scale as needed

void processInput(GLFWwindow* window);

void processInput(GLFWwindow* window) {
	const float deltaTime = 0.01f; // You can use a timer for frame-independent movement
	const float speed = 1.0f; // Reduced speed

	// Use the global spaceshipPosition variable
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		spaceshipPosition.y += speed * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		spaceshipPosition.y -= speed * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		spaceshipPosition.x -= speed * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		spaceshipPosition.x += speed * deltaTime;

	// Boundary checks
	const float max_x = 1.0f - spaceshipScale.x; // Assuming spaceship origin is at its center
	const float min_x = -1.0f + spaceshipScale.x;
	const float max_y = 1.0f - spaceshipScale.y;
	const float min_y = -1.0f + spaceshipScale.y;

	// Clamp the spaceship's position to the window boundaries
	spaceshipPosition.x = std::max(min_x, std::min(spaceshipPosition.x, max_x));
	spaceshipPosition.y = std::max(min_y, std::min(spaceshipPosition.y, max_y));
}



int main()
{
	// Initialize GLFW
	glfwInit();

	// Tell GLFW what version of OpenGL we are using 
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_DECORATED, GLFW_TRUE); // No window decorations for borderless window

	// Get the resolution of the primary monitor
	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);

	// Desired window size
	const int windowWidth = 800;
	const int windowHeight = 800;

	// Calculate the center position
	int monitorX, monitorY;
	glfwGetMonitorPos(monitor, &monitorX, &monitorY);

	int windowX = monitorX + (mode->width - windowWidth) / 2;
	int windowY = monitorY + (mode->height - windowHeight) / 2;

	// Create a windowed mode window with no decorations
	GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Space Explorer", NULL, NULL);
	if (!window) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	// Center the window on the screen
	glfwSetWindowPos(window, windowX, windowY);

	// Make the OpenGL context current
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable v-sync

	// Load GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// Enable blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Set up the viewport
	glViewport(0, 0, windowWidth, windowHeight);

	// Set up orthographic projection
	float aspectRatio = (float)windowWidth / (float)windowHeight;
	glm::mat4 projection = glm::ortho(-aspectRatio, aspectRatio, -1.0f, 1.0f);

		

	// Generates Shader object using shaders default.vert and default.frag
	Shader shaderProgram("defaultShader.vert", "defaultShader.frag");



	// Generates Vertex Array Object and binds it
	VAO VAO1;
	VAO1.Bind();

	// Generates Vertex Buffer Object and links it to vertices
	VBO VBO1(vertices, sizeof(vertices));
	// Generates Element Buffer Object and links it to indices
	EBO EBO1(indices, sizeof(indices));

	// Links VBO attributes such as coordinates and colors to VAO
	VAO1.LinkAttrib(VBO1, 0, 3, GL_FLOAT, 8 * sizeof(float), (void*)0);
	VAO1.LinkAttrib(VBO1, 1, 3, GL_FLOAT, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	VAO1.LinkAttrib(VBO1, 2, 2, GL_FLOAT, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	// Unbind all to prevent accidentally modifying them
	VAO1.Unbind();
	VBO1.Unbind();
	EBO1.Unbind();

	// Gets ID of uniform called "scale"
	GLuint uniID = glGetUniformLocation(shaderProgram.ID, "scale");

	// Texture
	Texture spaceShip("Assets/SpaceShip.png", GL_TEXTURE_2D, GL_TEXTURE0, GL_RGBA, GL_UNSIGNED_BYTE);
	spaceShip.texUnit(shaderProgram, "tex0", 0);

	// Original code from the tutorial
	/*Texture popCat("pop_cat.png", GL_TEXTURE_2D, GL_TEXTURE0, GL_RGBA, GL_UNSIGNED_BYTE);
	popCat.texUnit(shaderProgram, "tex0", 0);*/



	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		// Process input
		processInput(window);

		// Set background color and clear buffers
		glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// Activate shader and bind texture
		shaderProgram.Activate();
		spaceShip.Bind();

		// Create transformation matrix
		glm::mat4 transform = glm::mat4(1.0f);
		transform = glm::translate(transform, spaceshipPosition); // Move the spaceship
		transform = glm::scale(transform, spaceshipScale); // Scale the spaceship

		// Set the transformation matrix in the shader
		unsigned int transformLoc = glGetUniformLocation(shaderProgram.ID, "transform");
		glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));

		// Draw the spaceship
		VAO1.Bind();
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		// Swap buffers and poll events
		glfwSwapBuffers(window);
		glfwPollEvents();
	}




	// Delete all the objects we've created
	VAO1.Delete();
	VBO1.Delete();
	EBO1.Delete();
	spaceShip.Delete();
	shaderProgram.Delete();
	// Delete window before ending the program
	glfwDestroyWindow(window);
	// Terminate GLFW before ending the program
	glfwTerminate();
	return 0;
}