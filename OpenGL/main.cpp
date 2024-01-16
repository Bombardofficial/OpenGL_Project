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
#include <random>
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
	float radius;
	float speed; // Speed of the asteroid
	bool active;

	Asteroid(glm::vec3 pos, glm::vec3 scl, float rad, float spd)
		: position(pos), scale(scl), radius(rad), speed(spd), active(true) {}
};
struct Projectile {
	glm::vec3 position;
	glm::vec3 direction;
	float radius;
	float speed;
	bool active;

	Projectile(glm::vec3 pos, glm::vec3 dir, float rad, float spd)
		: position(pos), direction(dir), radius(rad), speed(spd), active(true) {}
};

std::vector<Projectile> projectiles;



//Global variables
std::vector<Asteroid> asteroids; // Vector to hold multiple asteroids
glm::vec3 spaceshipPosition = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 spaceshipScale = glm::vec3(0.2f, 0.2f, 0.2f); // Adjust scale as needed
float spaceshipRotation = 0.0f;
void processInput(GLFWwindow* window);
bool gameOver = false;
float spaceshipCollisionRadius = 0.01f; // Adjust based on spaceship size
float asteroidCollisionRadius = 0.02f; // Adjust based on asteroid size, might vary per asteroid
const float WORLD_SIZE_X = 50.0f; // Adjust as needed
const float WORLD_SIZE_Y = 50.0f; // Adjust as needed
// Desired window size
const int windowWidth = 1200;
const int windowHeight = 1200;
// Set up orthographic projection
float aspectRatio = (float)windowWidth / (float)windowHeight;
std::random_device rd;
std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
std::uniform_real_distribution<> disX;
std::uniform_real_distribution<> disY;
std::uniform_real_distribution<> disScale(0.2f, 0.7f); // Adjust the range as needed for asteroid scale
// Set the size of the visible area
const float VIEW_WIDTH = 2.5f; // The width of the visible area you want to see around the spaceship
const float VIEW_HEIGHT = VIEW_WIDTH / aspectRatio; // The height will depend on the aspect ratio to avoid stretching
bool isPaused = false;
bool escPreviouslyPressed = false;
const float PROJECTILE_COOLDOWN = 0.5f; // Cooldown time in seconds
float lastProjectileFireTime = 0.0f; // Time since the last projectile was fired


const int numberOfFireFrames = 4; // Number of fire frames
Texture* fireFrames[numberOfFireFrames];
float fireAnimationSpeed = 0.1f; // Time in seconds each frame is shown
float lastFrameTime = 0.0f; // Time when the last frame was changed
int currentFireFrame = 0; // Current frame index
bool isAccelerating = false; // Flag to check if spaceship is accelerating


void processInput(GLFWwindow* window) {
	
	const float deltaTime = 0.01f; // You can use a timer for frame-independent movement
	const float speed = 0.5f; // Reduced speed

	float adjustedRotation = spaceshipRotation + 90.0f; // Assumes spaceship faces right by default

	// Forward direction based on the spaceship's rotation
	glm::vec3 forward = glm::vec3(cos(glm::radians(adjustedRotation)), sin(glm::radians(adjustedRotation)), 0.0f);

	// Check if 'W' is pressed for forward movement
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		spaceshipPosition += speed * forward * deltaTime;
		isAccelerating = true; // Set isAccelerating to true when moving forward
	}
	else {
		isAccelerating = false; // Set isAccelerating to false when not moving forward
	}
	// Move backward
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		spaceshipPosition -= speed * forward * deltaTime;
	}
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
	//spaceshipPosition.x = std::max(min_x, std::min(spaceshipPosition.x, max_x));
	//spaceshipPosition.y = std::max(min_y, std::min(spaceshipPosition.y, max_y));

	// World wrapping logic
	if (spaceshipPosition.x < -WORLD_SIZE_X / 2) spaceshipPosition.x = WORLD_SIZE_X / 2;
	else if (spaceshipPosition.x > WORLD_SIZE_X / 2) spaceshipPosition.x = -WORLD_SIZE_X / 2;

	if (spaceshipPosition.y < -WORLD_SIZE_Y / 2) spaceshipPosition.y = WORLD_SIZE_Y / 2;
	else if (spaceshipPosition.y > WORLD_SIZE_Y / 2) spaceshipPosition.y = -WORLD_SIZE_Y / 2;

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		if (!escPreviouslyPressed) {
			isPaused = !isPaused;
			glfwSetInputMode(window, GLFW_CURSOR, isPaused ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
		}
		escPreviouslyPressed = true;
	}
	else {
		escPreviouslyPressed = false;
	}
	if (isPaused) {
		return; // Skip the rest of the input handling
	}
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
	static float lastX = 1200 / 2.0;
	static float lastY = 1200 / 2.0;

	// Sensitivity factor
	const float sensitivity = 0.07f;

	double offsetX = static_cast<float>(lastX - xpos) * sensitivity; // Invert the offsetX calculation
	double offsetY = static_cast<float>(lastY - ypos) * sensitivity; // Keep offsetY calculation as it is

	lastX = static_cast<float>(xpos);
	lastY = static_cast<float>(ypos);

	// Update spaceship rotation
	spaceshipRotation += static_cast<float>(offsetX);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		double currentTime = glfwGetTime();
		if (currentTime - lastProjectileFireTime >= PROJECTILE_COOLDOWN) {
			glm::vec3 projectileDir = glm::vec3(cos(glm::radians(spaceshipRotation + 90)), sin(glm::radians(spaceshipRotation + 90)), 0.0f);
			projectiles.push_back(Projectile(spaceshipPosition, projectileDir, 0.05f, 0.02f));
			lastProjectileFireTime = currentTime;
		}
	}
}

void updateProjectiles() {
	for (auto& projectile : projectiles) {
		if (projectile.active) {
			projectile.position += projectile.direction * projectile.speed;
			// Check if projectile is off-screen and deactivate it
			if (projectile.position.x < -WORLD_SIZE_X / 2 || projectile.position.x > WORLD_SIZE_X / 2 ||
				projectile.position.y < -WORLD_SIZE_Y / 2 || projectile.position.y > WORLD_SIZE_Y / 2) {
				projectile.active = false;
			}
		}
	}
	// Remove inactive projectiles
	projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(), [](const Projectile& p) { return !p.active; }), projectiles.end());
}

void checkProjectileAsteroidCollisions() {
	for (auto& projectile : projectiles) {
		if (!projectile.active) continue;
		for (auto& asteroid : asteroids) {
			if (!asteroid.active) continue;
			float distance = glm::distance(projectile.position, asteroid.position);
			if (distance < (projectile.radius + asteroid.radius)) {
				// Collision detected
				projectile.active = false;
				asteroid.active = false;
				// Optionally, add explosion effect or score increment here
				break; // No need to check other asteroids for this projectile
			}
		}
	}
	// Remove inactive projectiles and asteroids
	projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(), [](const Projectile& p) { return !p.active; }), projectiles.end());
	asteroids.erase(std::remove_if(asteroids.begin(), asteroids.end(), [](const Asteroid& a) { return !a.active; }), asteroids.end());
}


std::uniform_real_distribution<> disSpeed(0.005f, 0.015f);

void spawnAsteroid() {
	const float spawnMargin = 0.8f; // Adjust as needed

	// Calculate the spawn bounds
	float minX = std::max(spaceshipPosition.x - VIEW_WIDTH / 2 - spawnMargin, -WORLD_SIZE_X / 2);
	float maxX = std::min(spaceshipPosition.x + VIEW_WIDTH / 2 + spawnMargin, WORLD_SIZE_X / 2);
	float minY = std::max(spaceshipPosition.y - VIEW_HEIGHT / 2 - spawnMargin, -WORLD_SIZE_Y / 2);
	float maxY = std::min(spaceshipPosition.y + VIEW_HEIGHT / 2 + spawnMargin, WORLD_SIZE_Y / 2);

	// Generate a random position for the asteroid
	std::uniform_real_distribution<float> distX(minX, maxX);
	std::uniform_real_distribution<float> distY(minY, maxY);
	std::uniform_real_distribution<float> distScale(0.2f, 0.7f); // Adjust range as needed
	std::uniform_real_distribution<float> distSpeed(0.005f, 0.015f); // Range for asteroid speed

	float randX = distX(gen);
	float randY = distY(gen);

	// Ensure the asteroid isn't spawned within the viewable area
	while ((randX > spaceshipPosition.x - VIEW_WIDTH / 2) && (randX < spaceshipPosition.x + VIEW_WIDTH / 2) &&
		(randY > spaceshipPosition.y - VIEW_HEIGHT / 2) && (randY < spaceshipPosition.y + VIEW_HEIGHT / 2)) {
		randX = distX(gen);
		randY = distY(gen);
	}

	float randScale = distScale(gen);
	float radius = randScale * 0.2f; // Adjust this factor as needed
	float speed = distSpeed(gen); // Random speed for each asteroid

	// Push the new asteroid into the vector with a random speed
	asteroids.push_back(Asteroid(glm::vec3(randX, randY, 0.0f), glm::vec3(randScale, randScale, randScale), radius, speed));
}

bool checkCollision(const Asteroid& asteroid) {
	glm::vec2 spaceshipCenter(spaceshipPosition.x, spaceshipPosition.y);
	glm::vec2 asteroidCenter(asteroid.position.x, asteroid.position.y);

	float distance = glm::distance(spaceshipCenter, asteroidCenter);

	// Use asteroid's specific radius
	return distance < (spaceshipCollisionRadius + asteroid.radius);
}

void updateAsteroids() {
	for (auto& asteroid : asteroids) {
		asteroid.position.y -= asteroid.speed; // Use the asteroid's individual speed

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


	
	// Calculate the center position
	int monitorX, monitorY;
	glfwGetMonitorPos(monitor, &monitorX, &monitorY);

	int windowX = monitorX + (mode->width - windowWidth) / 2;
	int windowY = monitorY + (mode->height - windowHeight) / 2;

	double lastAsteroidSpawnIntervalUpdate = glfwGetTime();
	float asteroidSpawnInterval = 40.0f;

	// Create a windowed mode window with no decorations
	GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Space Explorer", NULL, NULL);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
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

	
	
	glm::mat4 projection = glm::ortho(-VIEW_WIDTH / 2, VIEW_WIDTH / 2, -VIEW_HEIGHT / 2, VIEW_HEIGHT / 2);

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
	asteroidTexture.texUnit(shaderProgram, "tex1", 0);

	Texture normalMap("Assets/NormalMap.png", GL_TEXTURE_2D, GL_TEXTURE1, GL_RGBA, GL_UNSIGNED_BYTE);
	normalMap.texUnit(shaderProgram, "normalMap", 1);

	Texture normalMap2("Assets/NormalMap2.png", GL_TEXTURE_2D, GL_TEXTURE2, GL_RGBA, GL_UNSIGNED_BYTE);
	normalMap2.texUnit(shaderProgram, "normalMap2", 3);

	Texture projectileTexture("Assets/Fx_02.png", GL_TEXTURE_2D, GL_TEXTURE0, GL_RGBA, GL_UNSIGNED_BYTE);
	projectileTexture.texUnit(shaderProgram, "tex2", 2);

	

	double lastTime = glfwGetTime();
	float lightIntensity = 1.0f;

	for (int i = 0; i < numberOfFireFrames; ++i) {
		fireFrames[i] = new Texture(("Assets/fire_" + std::to_string(i + 1) + ".png").c_str(), GL_TEXTURE_2D, GL_TEXTURE0 + 3 + i, GL_RGBA, GL_UNSIGNED_BYTE);
		fireFrames[i]->texUnit(shaderProgram, ("tex" + std::to_string(3 + i)).c_str(), 3 + i);
	}
	for (int i = 0; i < numberOfFireFrames; ++i) {
		shaderProgram.setInt("fireTextures[" + std::to_string(i) + "]", 3 + i);
	}


	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		
		// Process input
		processInput(window);
		
		// Start with an identity matrix each frame
		glm::mat4 view = glm::mat4(1.0f);

		// First, rotate the view matrix around the Z-axis to counteract the spaceship's rotation
		view = glm::rotate(view, glm::radians(-spaceshipRotation), glm::vec3(0.0f, 0.0f, 1.0f));

		// Then, translate the view matrix to counteract the spaceship's position
		view = glm::translate(view, glm::vec3(-spaceshipPosition.x, -spaceshipPosition.y, -1.0f));

		shaderProgram.Activate();

		// Pass the view matrix to the shader
		unsigned int viewLoc = glGetUniformLocation(shaderProgram.ID, "view");
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

		// Pass the projection matrix to the shader (this only needs to be done once unless the projection changes)
		unsigned int projLoc = glGetUniformLocation(shaderProgram.ID, "projection");
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

		// Disable the depth test
		glDisable(GL_DEPTH_TEST);

		// Update the projectile positions and check for collisions
		updateProjectiles();
		checkProjectileAsteroidCollisions();

		

		// Calculate time passed
		double currentTime = glfwGetTime();
		float deltaTime = static_cast<float>(currentTime - lastTime);
		lastTime = currentTime;

		// Update light intensity dynamically
		float frequency = 0.2f;
		float steepness = 3.0f;
		lightIntensity = 0.5f + 0.5f * sin(currentTime * frequency) * steepness;
			
		// Clamp the light intensity to stay within the range [0.0, 1.0]
		lightIntensity = 0.5f + 0.5f * static_cast<float>(sin(currentTime * frequency)) * steepness;

		// Update fire animation frame
		currentTime = glfwGetTime();
		if (currentTime - lastFrameTime >= fireAnimationSpeed && isAccelerating) {
			currentFireFrame = (currentFireFrame + 1) % numberOfFireFrames;
			lastFrameTime = currentTime;
		}
		// Activate shader
		shaderProgram.Activate();
		float lightDirectionX = static_cast<float>(sin(currentTime * 0.8f));
		// Set light direction and dynamic light intensity in shader
		glm::vec3 lightDir = glm::normalize(glm::vec3(lightDirectionX, -1.0f, -1.0f));
		GLint lightDirLoc = glGetUniformLocation(shaderProgram.ID, "lightDir");
		glUniform3f(lightDirLoc, lightDir.x, lightDir.y, lightDir.z);

		GLint lightIntensityLoc = glGetUniformLocation(shaderProgram.ID, "lightIntensity");
		glUniform1f(lightIntensityLoc, lightIntensity);

		if (!isPaused && !gameOver) {
			// Set background color and clear buffers
			glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			// Draw the spaceship
			shaderProgram.Activate();
			glActiveTexture(GL_TEXTURE0); // Activate the texture unit for the spaceship
			spaceShip.Bind();             // Bind the texture
			shaderProgram.setInt("tex0", 0);
			shaderProgram.setBool("isAsteroid", false);
			shaderProgram.setBool("isProjectile", false);
			shaderProgram.setBool("isFire", false); // Ensure the fire flag is set to false for spaceship
			glActiveTexture(GL_TEXTURE2); // Activate the texture unit for the spaceship normal map
			normalMap2.Bind();            // Bind the normal map
			shaderProgram.setInt("normalMap2", 2);

			// Create transformation matrix
			
			glm::mat4 spaceshipTransform = glm::mat4(1.0f);
			spaceshipTransform = glm::translate(spaceshipTransform, spaceshipPosition);
			spaceshipTransform = glm::scale(spaceshipTransform, spaceshipScale);
			spaceshipTransform = glm::rotate(spaceshipTransform, glm::radians(spaceshipRotation), glm::vec3(0.0f, 0.0f, 1.0f));
			unsigned int spaceshipTransformLoc = glGetUniformLocation(shaderProgram.ID, "transform");
			glUniformMatrix4fv(spaceshipTransformLoc, 1, GL_FALSE, glm::value_ptr(spaceshipTransform));

			// Draw the spaceship
			VAO1.Bind();
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

			if (isAccelerating) {
				// Activate the shader and set the current fire frame index
				shaderProgram.Activate();
				GLint currentFireFrameLocation = glGetUniformLocation(shaderProgram.ID, "currentFireFrame");
				glUniform1i(currentFireFrameLocation, currentFireFrame);

				// Start with an identity matrix
				glm::mat4 fireTransform = glm::mat4(1.0f);

				// First, translate the fire to be at the spaceship's position
				fireTransform = glm::translate(fireTransform, spaceshipPosition);

				// Then, apply the rotation to the fire to match the spaceship's rotation
				fireTransform = glm::rotate(fireTransform, glm::radians(spaceshipRotation), glm::vec3(0.0f, 0.0f, 1.0f));

				// Now translate the fire behind the spaceship, this will now use the rotated axis
				fireTransform = glm::translate(fireTransform, glm::vec3(0.0f, -0.065f, 0.0f)); // Adjust the offset if needed

				// Scale the fire as needed
				fireTransform = glm::scale(fireTransform, glm::vec3(0.035f, 0.035f, 0.035f));

				
				// Set the transformation matrix for the fire
				GLint fireTransformLoc = glGetUniformLocation(shaderProgram.ID, "transform");
				glUniformMatrix4fv(fireTransformLoc, 1, GL_FALSE, glm::value_ptr(fireTransform));

				// Activate the texture unit and bind the correct fire texture
				int fireTexUnit = 3 + currentFireFrame; // This will be 3, 4, 5, or 6
				glActiveTexture(GL_TEXTURE0 + fireTexUnit); // This activates the correct texture unit
				fireFrames[currentFireFrame]->Bind(); // Binds the correct fire frame texture
				glUniform1i(glGetUniformLocation(shaderProgram.ID, ("tex" + std::to_string(fireTexUnit)).c_str()), fireTexUnit);

				
				// Set flags for shader to indicate that fire is being drawn
				glUniform1i(glGetUniformLocation(shaderProgram.ID, "isFire"), (isAccelerating) ? GL_TRUE : GL_FALSE);
				glUniform1i(glGetUniformLocation(shaderProgram.ID, "isAsteroid"), GL_FALSE);
				glUniform1i(glGetUniformLocation(shaderProgram.ID, "isProjectile"), GL_FALSE);

				// Draw fire
				// Assuming that you have a quad VAO bound already that you can use to draw the fire
				VAO1.Bind();
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			}
			

			// Draw projectiles
			for (auto& projectile : projectiles) {
				if (!projectile.active) continue;
				shaderProgram.Activate();
				glActiveTexture(GL_TEXTURE2);
				projectileTexture.Bind();
				shaderProgram.setInt("tex2", 2);
				shaderProgram.setBool("isAsteroid", false);
				shaderProgram.setBool("isProjectile", true);
				glm::mat4 projectileTransform = glm::mat4(1.0f);
				projectileTransform = glm::translate(projectileTransform, projectile.position);
				projectileTransform = glm::scale(projectileTransform, glm::vec3(projectile.radius, projectile.radius, projectile.radius));
				projectileTransform = glm::rotate(projectileTransform, glm::radians(spaceshipRotation), glm::vec3(0.0f, 0.0f, 1.0f)); // Rotate the projectile
				unsigned int projectileTransformLoc = glGetUniformLocation(shaderProgram.ID, "transform");
				glUniformMatrix4fv(projectileTransformLoc, 1, GL_FALSE, glm::value_ptr(projectileTransform));
				VAO1.Bind();
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			}

			// Update the timer
			double currentTime = glfwGetTime();
			double deltaTime = currentTime - lastAsteroidSpawnIntervalUpdate;

			// Check if 10 seconds have passed
			if (deltaTime >= 10 && asteroidSpawnInterval > 10.0f)
			{
				// Decrease the spawn interval by a certain amount
				asteroidSpawnInterval -= 5.0f; // Decrease by 10 frames, adjust as needed

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

				shaderProgram.Activate();
				glActiveTexture(GL_TEXTURE1); // Activate the texture unit
				asteroidTexture.Bind();       // Bind the texture
				shaderProgram.setInt("tex1", 1);
				glActiveTexture(GL_TEXTURE2); // Activate the texture unit
				normalMap.Bind();             // Bind the texture
				shaderProgram.setInt("normalMap", 2);
				shaderProgram.setBool("isAsteroid", true);
				shaderProgram.setBool("isProjectile", false);

				// Similar to drawing the spaceship, create transformation matrices and draw each asteroid
				glm::mat4 asteroidTransform = glm::mat4(1.0f);
				asteroidTransform = glm::translate(asteroidTransform, asteroid.position);
				asteroidTransform = glm::scale(asteroidTransform, asteroid.scale);
				unsigned int asteroidTransformLoc = glGetUniformLocation(shaderProgram.ID, "transform");
				// Set transformation matrix in the shader and draw
				glUniformMatrix4fv(asteroidTransformLoc, 1, GL_FALSE, glm::value_ptr(asteroidTransform));
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			}
			
		}
		else if(gameOver){
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
	// Don't forget to deallocate memory at the end of your program
	for (int i = 0; i < numberOfFireFrames; ++i) {
		delete fireFrames[i];
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