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
#include <vector>
using namespace std;

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

struct Asteroid {
	glm::vec3 position;
	glm::vec3 scale;
	float radius; // Adding radius as a member variable

	Asteroid(glm::vec3 pos, glm::vec3 scl, float rad) : position(pos), scale(scl), radius(rad) {}
};

//Global variables
std::vector<Asteroid> asteroids; // Vector to hold multiple asteroids
glm::vec3 spaceshipPosition = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 spaceshipScale = glm::vec3(0.2f, 0.2f, 0.2f); // Adjust scale as needed
float spaceshipRotation = 0.0f;
void processInput(GLFWwindow* window);
bool gameOver = false;
float spaceshipCollisionRadius = 0.01f; // Adjust based on spaceship size
float asteroidCollisionRadius = 0.02f; // Adjust based on asteroid size, might vary per asteroid

void processInput(GLFWwindow* window) {
	
	const float deltaTime = 0.01f; // You can use a timer for frame-independent movement
	const float speed = 1.0f; // Reduced speed

	float adjustedRotation = spaceshipRotation + 90.0f; // Assumes spaceship faces right by default

	glm::vec3 forward = glm::vec3(cos(glm::radians(adjustedRotation)), sin(glm::radians(adjustedRotation)), 0.0f);
	glm::vec3 right = glm::vec3(-forward.y, forward.x, 0.0f);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		spaceshipPosition += speed * forward * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		spaceshipPosition -= speed * forward * deltaTime;
	/*if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		spaceshipPosition -= speed * right * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		spaceshipPosition += speed * right * deltaTime;*/

		// Boundary checks with a margin
	const float margin = -0.1f; // This will be subtracted from the max boundary and added to the min boundary
	const float max_x = (1.0f - margin) - spaceshipScale.x; // Right boundary
	const float min_x = (-1.0f + margin) + spaceshipScale.x; // Left boundary
	const float max_y = (1.0f - margin) - spaceshipScale.y; // Top boundary
	const float min_y = (-1.0f + margin) + spaceshipScale.y; // Bottom boundary

	// Clamp the spaceship's position to the window boundaries
	spaceshipPosition.x = std::max(min_x, std::min(spaceshipPosition.x, max_x));
	spaceshipPosition.y = std::max(min_y, std::min(spaceshipPosition.y, max_y));

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
	static double lastX = 800 / 2.0;
	static double lastY = 800 / 2.0;

	// Sensitivity factor
	const float sensitivity = 0.07f;

	double offsetX = (xpos - lastX) * sensitivity;
	double offsetY = (lastY - ypos) * sensitivity; // Reversed since y-coordinates range from bottom to top

	lastX = xpos;
	lastY = ypos;

	// Update spaceship rotation
	spaceshipRotation += offsetX;
}

void spawnAsteroid() {
	float randX = static_cast<float>(rand() % 200 - 100) / 100.0f; // Random x position
	float randY = 1.2f; // Y position
	float randScale = static_cast<float>(rand() % 50 + 20) / 100.0f; // Random scale

	// Approximate radius for the asteroid based on scale
	float radius = randScale * 0.2f; // Adjust this factor as needed
	asteroids.push_back(Asteroid(glm::vec3(randX, randY, 0.0f), glm::vec3(randScale, randScale, randScale), radius));
}

bool checkCollision(const Asteroid& asteroid) {
	glm::vec2 spaceshipCenter(spaceshipPosition.x, spaceshipPosition.y);
	glm::vec2 asteroidCenter(asteroid.position.x, asteroid.position.y);

	float distance = glm::distance(spaceshipCenter, asteroidCenter);

	// Use asteroid's specific radius
	return distance < (spaceshipCollisionRadius + asteroid.radius);
}

void updateAsteroids() {
	const float asteroidSpeed = 0.005f;

	for (auto& asteroid : asteroids) {
		asteroid.position.y -= asteroidSpeed;

		if (checkCollision(asteroid)) {
			gameOver = true;
			break;
		}
	}
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

	double lastAsteroidSpawnIntervalUpdate = glfwGetTime();
	float asteroidSpawnInterval = 100.0f;

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
	// Set the mouse callback
	glfwSetCursorPosCallback(window, mouse_callback);

	// Hide the cursor and capture it
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
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

	Texture asteroidTexture("Assets/Asteroid.png", GL_TEXTURE_2D, GL_TEXTURE0, GL_RGBA, GL_UNSIGNED_BYTE);
	asteroidTexture.texUnit(shaderProgram, "tex0", 0);

	// Original code from the tutorial
	/*Texture popCat("pop_cat.png", GL_TEXTURE_2D, GL_TEXTURE0, GL_RGBA, GL_UNSIGNED_BYTE);
	popCat.texUnit(shaderProgram, "tex0", 0);*/



	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		// Process input
		processInput(window);

		if (!gameOver) {
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
			transform = glm::rotate(transform, glm::radians(spaceshipRotation), glm::vec3(0.0f, 0.0f, 1.0f)); // rotate the spaceship
			// Set the transformation matrix in the shader
			unsigned int transformLoc = glGetUniformLocation(shaderProgram.ID, "transform");
			glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));

			// Draw the spaceship
			VAO1.Bind();
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

			// Update the timer
			double currentTime = glfwGetTime();
			double deltaTime = currentTime - lastAsteroidSpawnIntervalUpdate;

			// Check if 10 seconds have passed
			if (deltaTime >= 10 && asteroidSpawnInterval > 20.0f)
			{
				// Decrease the spawn interval by a certain amount
				asteroidSpawnInterval -= 10.0f; // Decrease by 10 frames, adjust as needed

				lastAsteroidSpawnIntervalUpdate = currentTime;
			}

			// Spawn asteroids at intervals or as needed
			// Example: spawn an asteroid every 100 frames
			static int frameCount = 0;
			if (frameCount % static_cast<int>(asteroidSpawnInterval) == 0) {
				spawnAsteroid();
			}
			frameCount++;

			// Update asteroid positions
			updateAsteroids();

			// Draw asteroids
			for (auto& asteroid : asteroids) {

				// Activate shader and bind asteroid texture
				shaderProgram.Activate();
				asteroidTexture.Bind(); // Use the asteroid texture

				// Similar to drawing the spaceship, create transformation matrices and draw each asteroid
				glm::mat4 asteroidTransform = glm::mat4(1.0f);
				asteroidTransform = glm::translate(asteroidTransform, asteroid.position);
				asteroidTransform = glm::scale(asteroidTransform, asteroid.scale);

				// Set transformation matrix in the shader and draw
				glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(asteroidTransform));
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			}

		}
		else {
			// Render game over screen
			// Clear screen
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black background
			glClear(GL_COLOR_BUFFER_BIT);

			// Render "Game Over" text
			// This can be done using text rendering libraries like FreeType
			// For simplicity, you might just display a black screen here
		}
		
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