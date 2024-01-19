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
#include <glm/gtx/string_cast.hpp>
#include <vector>
#include <random>


using namespace std;


template <typename T>
T clamp(T value, T min, T max) {
	return (value < min) ? min : (value > max) ? max : value;
}

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
const int numberOfExplosionFrames = 5; // Number of explosion frames
Texture* explosionFrames[numberOfExplosionFrames];
float explosionAnimationSpeed = 0.05f; // Time in seconds each frame is shown
struct Asteroid {
	glm::vec3 position;
	glm::vec3 scale;
	float radius;
	float speed; // Speed of the asteroid
	bool active;
	bool isExploding = false;
	float explosionAnimationTime = 0.0f; // Current time in the explosion animation
	int currentExplosionFrame = 0;
	float explosionFixedDeltaTime = 1.0f / 100.0f;
	Asteroid(glm::vec3 pos, glm::vec3 scl, float rad, float spd)
		: position(pos), scale(scl), radius(rad), speed(spd), active(true), isExploding(false) {}

	void update(float deltaTime) {
		if (isExploding) {
			explosionAnimationTime += explosionFixedDeltaTime;
			if (explosionAnimationTime >= explosionAnimationSpeed) {
				if (currentExplosionFrame < numberOfExplosionFrames - 1) {
					currentExplosionFrame++;
				}
				else {
					// Last frame already displayed, so mark the asteroid inactive
					active = false;
					isExploding = false;
					return; // Important: Return here to avoid further processing
				}
				explosionAnimationTime = 0.0f;
			}
		}
		else {
			position.y -= speed; // Normal movement
		}

	}
};
enum class Owner {
	Player,
	Enemy
};

struct Projectile {
	glm::vec3 position;
	glm::vec3 direction;
	float radius;
	float speed;
	bool active;
	Owner owner; // Add this field
	float collisionRadius = 0.1f; // Adjust this as needed
	float rotation;

	Projectile(glm::vec3 pos, glm::vec3 dir, float rad, float spd, float colRad, Owner own)
		: position(pos), direction(dir), radius(rad), speed(spd), collisionRadius(colRad), active(true), owner(own), rotation(0.0f) {
		// Calculate the initial rotation based on the direction
		rotation = glm::degrees(atan2(direction.y, direction.x));
	}
};

std::vector<Projectile> projectiles;
//Global variables
std::vector<Asteroid> asteroids; // Vector to hold multiple asteroids
glm::vec3 spaceshipPosition = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 spaceshipScale = glm::vec3(0.2f, 0.2f, 0.2f); // Adjust scale as needed
float spaceshipRotation = 0.0f;
void processInput(GLFWwindow* window);
bool gameOver = false;
float spaceshipCollisionRadius = 0.02f; // Adjust based on spaceship size
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
double gameStartTime = glfwGetTime();
int spaceshipLives = 2; // Start with two lives
const int numberOfFireFrames = 4; // Number of fire frames
Texture* fireFrames[numberOfFireFrames];

float fireAnimationSpeed = 0.1f; // Time in seconds each frame is shown


float lastFrameTime = 0.0f; // Time when the last frame was changed
int currentFireFrame = 0; // Current frame index
bool isAccelerating = false; // Flag to check if spaceship is accelerating
float enemyCollisionRadius = 0.1f; // Adjust based on enemy size

const float ENEMY_SPAWN_COOLDOWN = 5.0f; // Spawn an enemy every 5 seconds
float lastEnemySpawnTime = 0.0f;

bool bigEnemySpawned = false;
double lastBigEnemySpawnTime = 0.0;
bool bossEnemySpawned = false;
double lastBossEnemySpawnTime = 0.0;

double lastAsteroidSpawnIntervalUpdate = glfwGetTime();
float asteroidSpawnInterval = 60.0f;



// Function to generate a random float between two values
float randomFloat(float a, float b) {
	return a + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (b - a)));
}
struct Enemy {
	glm::vec3 position;
	glm::vec3 direction;
	float speed;
	bool active;
	float radius;
	float lastShootTime;
	float rotation;
	float minimumDistance = 0.5f; // Minimum distance to the player
	float shootingRange; // Maximum distance for shooting at the player
	float stoppingDistance; // Distance to stop moving towards the player
	float shootCooldown; // New member for individual shooting cooldown
	bool isExploding = false;
	bool isBig;
	bool isBoss;
	int lives;
	Enemy(glm::vec3 pos, float spd, float rad = 0.1f, bool big = false, bool boss = false)
		: position(pos), speed(spd), active(true), radius(rad), lastShootTime(0.0f), rotation(0.0f), isBig(big), isBoss(boss) {
		if (isBoss) {
			lives = 10;
			shootingRange = VIEW_WIDTH / 2; // Increase shooting range
			minimumDistance = 2.0f; // Increase minimum distance
			stoppingDistance = randomFloat(minimumDistance, shootingRange); // Increase stopping distance
			shootCooldown = randomFloat(2.0f, 4.0f); // Adjust cooldown as needed
		}
		else if (isBig) {
			lives = 3;
			shootingRange = VIEW_WIDTH / 2; // Increase shooting range
			minimumDistance = 1.0f; // Increase minimum distance
			stoppingDistance = randomFloat(minimumDistance, shootingRange); // Increase stopping distance
			shootCooldown = randomFloat(2.0f, 4.0f); // Adjust cooldown as needed
		}
		else {
			lives = 1;
			shootingRange = VIEW_WIDTH / 3; // Adjust as needed
			minimumDistance = 0.5f; // Adjust as needed
			stoppingDistance = randomFloat(minimumDistance, shootingRange); // Adjust as needed
			shootCooldown = randomFloat(1.0f, 2.0f); // Adjust as needed
		}
		direction = glm::normalize(spaceshipPosition - pos);
	}

	void update(float deltaTime) {
		if (!isPaused) {
			glm::vec3 toPlayer = spaceshipPosition - position;
			float distanceToPlayer = glm::length(toPlayer);

			// Move towards the player only if outside of stoppingDistance
			if (distanceToPlayer > stoppingDistance) {
				direction = glm::normalize(toPlayer);
				position += direction * speed * deltaTime;
			}
			else {
				direction = glm::vec3(0.0f); // Stop moving
			}

			// Update rotation to face the player
			rotation = glm::degrees(atan2(toPlayer.y, toPlayer.x)) - 90.0f;
		}
		
	}
	
};
std::vector<Enemy> enemies;

float boostCharge = 1.0f; // Fully charged
const float boostDrain = 0.1f; // Drain per second
const float boostRecharge = 0.1f; // Recharge per second
const float boostSpeed = 0.5f; // Additional speed when boosting

void initializeGameState(std::vector<Projectile>& projectiles,
	std::vector<Asteroid>& asteroids,
	std::vector<Enemy>& enemies,
	glm::vec3& spaceshipPosition,
	int& spaceshipLives) {
	// Reset spaceship state
	spaceshipPosition = glm::vec3(0.0f, 0.0f, 0.0f);
	spaceshipLives = 2;
	spaceshipRotation = 0.0f;  // Reset spaceship rotation
	isAccelerating = false;    // Reset spaceship acceleration state

	// Reset boost charge
	boostCharge = 1.0f; // Fully charged

	// Clear existing game objects
	projectiles.clear();
	asteroids.clear();
	enemies.clear();

	// Reset enemy spawn timers
	lastEnemySpawnTime = 0.0f;
	lastBigEnemySpawnTime = 0.0f;
	lastBossEnemySpawnTime = 0.0f;

	// Reset game control flags
	gameOver = false;
	isPaused = false;

	// Reset asteroid spawn interval
	asteroidSpawnInterval = 60.0f;
	lastAsteroidSpawnIntervalUpdate = glfwGetTime();  // Reset interval update timer

	// Reset game start time
	gameStartTime = glfwGetTime();

	// Add any additional state resets here...
}



void processInput(GLFWwindow* window) {
	const float baseSpeed = 0.2f; // Base speed for automatic forward movement
	float deltaTime	= 0.01f;
	float adjustedRotation = spaceshipRotation + 90.0f;
	glm::vec3 forward = glm::vec3(cos(glm::radians(adjustedRotation)), sin(glm::radians(adjustedRotation)), 0.0f);

	if (gameOver && glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
		// Reset game state to initial conditions
		gameOver = false;
		initializeGameState(projectiles, asteroids, enemies, spaceshipPosition, spaceshipLives);

		// You may need to reset other states, like score, timers, etc.
		// ...
	}
	// Check if 'W' is pressed for boost
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS && boostCharge > 0) {
		spaceshipPosition += boostSpeed * forward * deltaTime;
		boostCharge -= boostDrain * deltaTime;
		isAccelerating = true;
	}
	else {
		boostCharge += boostRecharge * deltaTime;
		isAccelerating = false;
	}

	// Clamp boost charge between 0 and 1
	boostCharge = clamp(boostCharge, 0.0f, 1.0f);
	// Move backward
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		spaceshipPosition -= baseSpeed * forward * deltaTime;
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
			glm::vec3 projectileDirection = glm::vec3(cos(glm::radians(spaceshipRotation + 90)), sin(glm::radians(spaceshipRotation + 90)), 0.0f);
			projectiles.push_back(Projectile(spaceshipPosition, projectileDirection, 0.05f, 0.02f, 0.01f, Owner::Player));
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
		if (!projectile.active || projectile.owner != Owner::Player) continue; // Only check player projectiles

		for (auto& asteroid : asteroids) {
			if (!asteroid.active) continue;
			float distance = glm::distance(projectile.position, asteroid.position);
			if (distance < (projectile.collisionRadius + asteroid.radius)) {
				projectile.active = false;
				asteroid.isExploding = true; // Start the explosion
				asteroid.explosionAnimationTime = 0.0f; // Reset the explosion animation time
				asteroid.currentExplosionFrame = 0; // Start from the first frame
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

		// Check for collision only if the asteroid is active and not exploding
		if (asteroid.active && !asteroid.isExploding && checkCollision(asteroid)) {
			gameOver = true;
			break;
		}
	}
}



void spawnEnemy(bool isBig, bool isBoss) {
	// Define the offset limits for spawning enemies
	float spawnOffsetX = VIEW_WIDTH / 2 + 1.0f; // Spawn beyond half the view width
	float spawnOffsetY = VIEW_HEIGHT / 2 + 1.0f; // Spawn beyond half the view height

	std::uniform_real_distribution<float> distX(-spawnOffsetX, spawnOffsetX);
	std::uniform_real_distribution<float> distY(-spawnOffsetY, spawnOffsetY);

	glm::vec3 enemyPos;
	do {
		float offsetX = distX(gen);
		float offsetY = distY(gen);
		enemyPos = spaceshipPosition + glm::vec3(offsetX, offsetY, 0.0f);
	} while (glm::distance(enemyPos, spaceshipPosition) < spawnOffsetX); // Ensure the enemy is outside the view

	float speed = 0.02f; // Adjust the speed as needed
	Enemy newEnemy(enemyPos, speed, enemyCollisionRadius, isBig, isBoss);
	newEnemy.radius = enemyCollisionRadius + 0.2f;
	newEnemy.lastShootTime = glfwGetTime();
	// Set a random minimum distance for each enemy to prevent clustering
	std::uniform_real_distribution<float> distMin(0.5f, VIEW_WIDTH / 4);
	newEnemy.minimumDistance = distMin(gen);
	if (isBig) {
		newEnemy.radius = enemyCollisionRadius + 0.4f; // This should be defined somewhere
	} else if (isBoss) {
		newEnemy.radius = enemyCollisionRadius + 1.5f;
	}
	enemies.push_back(newEnemy);
}

void enemyShoot(Enemy& enemy) {
	// Calculate distance from the enemy to the spaceship
	float shootDistance = glm::distance(enemy.position, spaceshipPosition);
	// Debugging statement to check shooting conditions

	// Check if the enemy can shoot based on the distance and cooldown
	if (shootDistance <= enemy.shootingRange && glfwGetTime() - enemy.lastShootTime >= enemy.shootCooldown) {
		// Calculate the projectile direction towards the player with some randomness
		glm::vec3 projectileDirection = glm::normalize(spaceshipPosition - enemy.position);
		float offsetAngleDegrees = randomFloat(-5.0f, 5.0f); // Random angle between -5 and +5 degrees for each shot
		float offsetAngleRadians = glm::radians(offsetAngleDegrees);
		glm::mat2 rotationMatrix = glm::mat2(cos(offsetAngleRadians), -sin(offsetAngleRadians), sin(offsetAngleRadians), cos(offsetAngleRadians));
		glm::vec2 rotatedDirection = rotationMatrix * glm::vec2(projectileDirection.x, projectileDirection.y);
		projectileDirection.x = rotatedDirection.x;
		projectileDirection.y = rotatedDirection.y;
		// Define projectile characteristics based on enemy type
		float projectileSpeed = 0.02f; // Common speed for all projectiles
		float projectileRadius = 0.05f; // Default radius
		if (enemy.isBoss) {
			projectileRadius = 0.08f; // Larger radius for boss
		}
		else if (enemy.isBig) {
			projectileRadius = 0.07f; // Slightly larger radius for big enemies
		}

		// Create and push the projectile
		projectiles.push_back(Projectile(enemy.position, projectileDirection, projectileRadius, projectileSpeed, 0.01f, Owner::Enemy));

		// Update the last shoot time
		enemy.lastShootTime = static_cast<float>(glfwGetTime());
	}
}




float enemyProjectileCollisionRadius = 0.1f;
void checkEnemyProjectileCollisions(Texture& enemyExplosionSpritesheet) {
	float minusenemycolliderradius;
	for (auto& projectile : projectiles) {
		if (!projectile.active) continue; // Skip inactive projectiles

		// Check collision with enemies for player projectiles
		if (projectile.owner == Owner::Player) {
			for (auto& enemy : enemies) {
				if (!enemy.active) continue; // Skip inactive enemies
				if (enemy.isBig) {
					minusenemycolliderradius = 0.28f;
				}
				else if (enemy.isBoss) {
					minusenemycolliderradius = 1.0f;
				}
				else {
					minusenemycolliderradius = 0.25f;
				}
				float distance = glm::distance(projectile.position, enemy.position);
				if (distance < (projectile.collisionRadius + enemy.radius - minusenemycolliderradius)) {
					projectile.active = false;
					enemy.lives--; // Decrement enemy's lives

					if (enemy.lives <= 0) {
						enemy.active = false;
						// Additional logic like score increment can be added here
					}

					break; // Exit loop after collision is found
				}
			}
		}
		else if (projectile.owner == Owner::Enemy) {
			if (glm::distance(projectile.position, spaceshipPosition) < (spaceshipCollisionRadius + projectile.collisionRadius)) {
				projectile.active = false; // Deactivate the projectile
				spaceshipLives--; // Decrement the spaceship's lives
				if (spaceshipLives <= 0) {
					gameOver = true;
				}
				break;
			}
		}
	}
	// Remove inactive projectiles
	projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(), [](const Projectile& p) { return !p.active; }), projectiles.end());
}



void checkProjectilePlayerCollision() {
	
	for (auto& projectile : projectiles) {
		if (!projectile.active || projectile.owner == Owner::Player) continue; // Ignore inactive or player-owned projectiles

		// Check collision only for enemy projectiles
		if (projectile.owner == Owner::Enemy) {
			if (glm::distance(projectile.position, spaceshipPosition) < (spaceshipCollisionRadius + projectile.collisionRadius)) {
				gameOver = true;
				projectile.active = false; // Deactivate the projectile
				break;
			}
		}
	}
}

void updateEnemies(float deltaTime) {
	if (isPaused) {
		
		return;
	}
	for (auto& enemy : enemies) {
		if (!enemy.active) continue;

		glm::vec3 toPlayer = spaceshipPosition - enemy.position;
		float distanceToPlayer = glm::length(toPlayer);

		// Increase the speed for more noticeable movement
		float adjustedSpeed = 0.3f; // Adjust this value as needed

		if (distanceToPlayer > enemy.stoppingDistance) {
			enemy.direction = glm::normalize(toPlayer);
			enemy.position += enemy.direction * adjustedSpeed * deltaTime; // Use adjustedSpeed
		}
		else {
			enemy.direction = glm::vec3(0.0f); // Stop moving
		}

		// Update rotation to face the player
		enemy.rotation = glm::degrees(atan2(toPlayer.y, toPlayer.x)) - 90.0f;

		// Check for shooting condition
		if (glfwGetTime() - enemy.lastShootTime >= enemy.shootCooldown) {
			enemyShoot(enemy);
		}
	}
}


int main()
{
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

	Texture spaceShip2("Assets/SpaceShip2.png", GL_TEXTURE_2D, GL_TEXTURE13, GL_RGBA, GL_UNSIGNED_BYTE);
	spaceShip2.texUnit(shaderProgram, "spaceshiponelife", 13);

	Texture asteroidTexture("Assets/Asteroid.png", GL_TEXTURE_2D, GL_TEXTURE0, GL_RGBA, GL_UNSIGNED_BYTE);
	asteroidTexture.texUnit(shaderProgram, "tex1", 0);

	Texture normalMap("Assets/NormalMap.png", GL_TEXTURE_2D, GL_TEXTURE1, GL_RGBA, GL_UNSIGNED_BYTE);
	normalMap.texUnit(shaderProgram, "normalMap", 1);

	Texture normalMap2("Assets/NormalMap2.png", GL_TEXTURE_2D, GL_TEXTURE2, GL_RGBA, GL_UNSIGNED_BYTE);
	normalMap2.texUnit(shaderProgram, "normalMap2", 3);

	Texture normalMap5("Assets/NormalMap5.png", GL_TEXTURE_2D, GL_TEXTURE14, GL_RGBA, GL_UNSIGNED_BYTE);
	normalMap5.texUnit(shaderProgram, "normalMap5", 14);

	Texture projectileTexture("Assets/Fx_02.png", GL_TEXTURE_2D, GL_TEXTURE0, GL_RGBA, GL_UNSIGNED_BYTE);
	projectileTexture.texUnit(shaderProgram, "tex2", 2);

	Texture enemyTexture("Assets/enemy2.png", GL_TEXTURE_2D, GL_TEXTURE0, GL_RGBA, GL_UNSIGNED_BYTE);
	enemyTexture.texUnit(shaderProgram, "tex7", 7);

	Texture enemyNormalMap("Assets/NormalMap3.png", GL_TEXTURE_2D, GL_TEXTURE1, GL_RGBA, GL_UNSIGNED_BYTE);
	enemyNormalMap.texUnit(shaderProgram, "normalMap3", 11);

	Texture bigEnemyNormalMap("Assets/NormalMap4.png", GL_TEXTURE_2D, GL_TEXTURE1, GL_RGBA, GL_UNSIGNED_BYTE);
	bigEnemyNormalMap.texUnit(shaderProgram, "normalMap4", 12);

	Texture enemyExplosionSpritesheet("Assets/EnemyExplosion.png", GL_TEXTURE_2D, GL_TEXTURE10, GL_RGBA, GL_UNSIGNED_BYTE);
	enemyExplosionSpritesheet.texUnit(shaderProgram, "texEnemyExplosion", 10);

	// Define a new texture for the big enemy
	Texture bigEnemyTexture("Assets/BigEnemy.png", GL_TEXTURE_2D, GL_TEXTURE8, GL_RGBA, GL_UNSIGNED_BYTE);
	bigEnemyTexture.texUnit(shaderProgram, "tex8", 8);

	Texture bossEnemyTexture("Assets/BossEnemy.png", GL_TEXTURE_2D, GL_TEXTURE15, GL_RGBA, GL_UNSIGNED_BYTE);
	bossEnemyTexture.texUnit(shaderProgram, "tex15", 15);

	Texture bossEnemyNormalMap("Assets/NormalMap6.png", GL_TEXTURE_2D, GL_TEXTURE16, GL_RGBA, GL_UNSIGNED_BYTE);
	bossEnemyNormalMap.texUnit(shaderProgram, "normalMap6", 16);

	double lastTime = glfwGetTime();
	float lightIntensity = 1.0f;

	for (int i = 0; i < numberOfFireFrames; ++i) {
		fireFrames[i] = new Texture(("Assets/fire_" + std::to_string(i + 1) + ".png").c_str(), GL_TEXTURE_2D, GL_TEXTURE0 + 3 + i, GL_RGBA, GL_UNSIGNED_BYTE);
		fireFrames[i]->texUnit(shaderProgram, ("tex" + std::to_string(3 + i)).c_str(), 3 + i);
	}
	for (int i = 0; i < numberOfFireFrames; ++i) {
		shaderProgram.setInt("fireTextures[" + std::to_string(i) + "]", 3 + i);
	}
	for (int i = 0; i < numberOfExplosionFrames; ++i) {
		explosionFrames[i] = nullptr;
	}
	for (int i = 0; i < numberOfExplosionFrames; ++i) {
		explosionFrames[i] = new Texture(("Assets/asteroidexp" + std::to_string(i + 1) + ".png").c_str(), GL_TEXTURE_2D, GL_TEXTURE0 + 17 + i, GL_RGBA, GL_UNSIGNED_BYTE);
		if (explosionFrames[i] != nullptr) {
			std::cerr << "Failed to load texture for frame " << i << std::endl;
			// Handle the error, possibly by setting the pointer to nullptr or exiting
		}
		explosionFrames[i]->texUnit(shaderProgram, ("tex" + std::to_string(17 + i)).c_str(), 17 + i);
	}
	for (int i = 0; i < numberOfExplosionFrames; ++i) {
		shaderProgram.setInt("explosionTextures[" + std::to_string(i) + "]", 17 + i);
	}



	srand(static_cast<unsigned int>(time(nullptr)));

	std::uniform_real_distribution<> spawnTimeDist(5.0, 15.0); // Distribution for random spawn time
	double nextSpawnTime = lastTime + spawnTimeDist(gen);
	//Explosion testExplosion(glm::vec3(0.0f, 0.0f, 0.0f), createAnimationFromSpritesheet(asteroidExplosionSpritesheet, 8, 0.1f));

	double nextBigEnemySpawnTime = 0.0;
	double nextBossEnemySpawnTime = 0.0;
	float enemyandspaceshipcollisionradius;
	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		
		// Process input
		processInput(window);
		
		if (isPaused) {
			glfwPollEvents();
			continue;
		}

		


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
		// Calculate time passed
		double currentTime = glfwGetTime();
		float deltaTime = static_cast<float>(currentTime - lastTime);
		lastTime = currentTime;

		
		if (currentTime >= nextSpawnTime) {
			spawnEnemy(false,false); // Spawn normal enemy with false flag
			nextSpawnTime = currentTime + spawnTimeDist(gen); // Set next spawn time
		}

		if (!isPaused) {
			// Update the projectile positions and check for collisions
			updateProjectiles();
			checkProjectileAsteroidCollisions();
			checkEnemyProjectileCollisions(enemyExplosionSpritesheet);
			updateEnemies(deltaTime);
			// Update asteroid positions
			updateAsteroids();
			
			


		}

		

		// Update light intensity dynamically
		float frequency = 0.15f;
		float steepness = 3.0f;
		lightIntensity = 1.0f + 0.5f * static_cast<float>(sin(currentTime * frequency)) * steepness;


		if (currentTime - lastFrameTime >= fireAnimationSpeed && isAccelerating) {
			currentFireFrame = (currentFireFrame + 1) % numberOfFireFrames;
			lastFrameTime = static_cast<float>(currentTime);
		}
		// Activate shader
		shaderProgram.Activate();
		float lightDirectionX = static_cast<float>(sin(currentTime * 0.2f));
		// Set light direction and dynamic light intensity in shader
		glm::vec3 lightDir = glm::normalize(glm::vec3(lightDirectionX, -1.0f, -1.0f));
		GLint lightDirLoc = glGetUniformLocation(shaderProgram.ID, "lightDir");
		glUniform3f(lightDirLoc, lightDir.x, lightDir.y, lightDir.z);

		GLint lightIntensityLoc = glGetUniformLocation(shaderProgram.ID, "lightIntensity");
		glUniform1f(lightIntensityLoc, lightIntensity);


		if (!gameOver) {
			// Set background color and clear buffers
			glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			checkProjectilePlayerCollision();
			// Draw the spaceship
			shaderProgram.Activate();
			if (spaceshipLives == 2) {
				glActiveTexture(GL_TEXTURE0); // Activate the texture unit for the spaceship
				spaceShip.Bind();             // Bind the texture
				shaderProgram.setInt("tex0", 0);
				glActiveTexture(GL_TEXTURE2); // Activate the texture unit for the spaceship normal map
				normalMap2.Bind();            // Bind the normal map
				shaderProgram.setInt("normalMap2", 2);
				shaderProgram.setInt("spaceshipHealth", 2);
			}
			else{
				glActiveTexture(GL_TEXTURE13); // Activate the texture unit for the spaceship
				spaceShip2.Bind();             // Bind the texture
				shaderProgram.setInt("spaceshiponelife", 13);
				glActiveTexture(GL_TEXTURE14); // Activate the texture unit for the spaceship normal map
				normalMap5.Bind();            // Bind the normal map
				shaderProgram.setInt("normalMap5", 14);
				shaderProgram.setInt("spaceshipHealth", 1);
			}
			shaderProgram.setBool("isAsteroid", false);
			shaderProgram.setBool("isProjectile", false);
			shaderProgram.setBool("isFire", false); // Ensure the fire flag is set to false for spaceship
			shaderProgram.setBool("isEnemy", false);
			

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
				shaderProgram.setBool("isEnemy", false);
				shaderProgram.setBool("isFire", false);
				glm::mat4 projectileTransform = glm::mat4(1.0f);
				projectileTransform = glm::translate(projectileTransform, projectile.position);
				projectileTransform = glm::scale(projectileTransform, glm::vec3(projectile.radius, projectile.radius, projectile.radius));

				// Apply rotation
				projectileTransform = glm::rotate(projectileTransform, glm::radians(projectile.rotation), glm::vec3(0.0f, 0.0f, 1.0f));

				unsigned int projectileTransformLoc = glGetUniformLocation(shaderProgram.ID, "transform");
				glUniformMatrix4fv(projectileTransformLoc, 1, GL_FALSE, glm::value_ptr(projectileTransform));
				VAO1.Bind();
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			}

			if (currentTime - lastEnemySpawnTime >= ENEMY_SPAWN_COOLDOWN) {
				std::cout << "Spawning enemy at time: " << currentTime << std::endl; // Debug statement
				spawnEnemy(false,false); // Call the function to spawn an enemy
				lastEnemySpawnTime = static_cast<float>(currentTime); // Reset the timer
			}

			if (!bigEnemySpawned) {
				if (currentTime - gameStartTime >= randomFloat(30.0f, 45.0f)) {
					spawnEnemy(true,false); // Spawn first big enemy
					bigEnemySpawned = true;
					lastBigEnemySpawnTime = currentTime;
					nextBigEnemySpawnTime = randomFloat(30.0f, 60.0f); // Set the next big enemy spawn time
				}
			}
			else if (currentTime - lastBigEnemySpawnTime >= nextBigEnemySpawnTime) {
				spawnEnemy(true,false); // Spawn subsequent big enemies
				lastBigEnemySpawnTime = currentTime;
				nextBigEnemySpawnTime = randomFloat(30.0f, 60.0f); // Calculate the next spawn time
			}

			if(!bossEnemySpawned){
				if (currentTime - gameStartTime >= randomFloat(90.0f, 120.0f)) {
					spawnEnemy(false,true); // Spawn first boss enemy
					bossEnemySpawned = true;
					lastBossEnemySpawnTime = currentTime;
					nextBossEnemySpawnTime = randomFloat(60.0f, 90.0f); // Set the next boss enemy spawn time
				}
			}
			else if (currentTime - lastBossEnemySpawnTime >= nextBossEnemySpawnTime) {
				spawnEnemy(false,true); // Spawn subsequent boss enemies
				lastBossEnemySpawnTime = currentTime;
				nextBossEnemySpawnTime = randomFloat(60.0f, 90.0f); // Calculate the next spawn time
			}


			for (auto& enemy : enemies) {
				if (enemy.active) {
					shaderProgram.Activate();
					shaderProgram.setBool("isBigEnemy", enemy.isBig); // Set isBigEnemy uniform based on the enemy's isBig property
					shaderProgram.setBool("isBossEnemy", enemy.isBoss);
					if (enemy.isBig) {
						// Bind the big enemy texture
						glActiveTexture(GL_TEXTURE8); // Texture unit for the big enemy
						bigEnemyTexture.Bind();
						shaderProgram.setInt("tex8", 8);
						glActiveTexture(GL_TEXTURE12); // Texture unit for the big enemy normal map
						bigEnemyNormalMap.Bind();
						shaderProgram.setInt("normalMap4", 12);

					} else if(enemy.isBoss) {
						// Bind the boss enemy texture
						glActiveTexture(GL_TEXTURE15); // Texture unit for the boss enemy
						bossEnemyTexture.Bind();
						shaderProgram.setInt("tex15", 15);
						glActiveTexture(GL_TEXTURE16); // Texture unit for the boss enemy normal map
						bossEnemyNormalMap.Bind();
						shaderProgram.setInt("normalMap6", 16);
					}
					else {
						// Bind the normal enemy texture
						glActiveTexture(GL_TEXTURE7); // Texture unit for the normal enemy
						enemyTexture.Bind();
						shaderProgram.setInt("tex7", 7);
						glActiveTexture(GL_TEXTURE11); // Texture unit for the normal enemy normal map
						enemyNormalMap.Bind();
						shaderProgram.setInt("normalMap3", 11);
					}
					
					//glActiveTexture(GL_TEXTURE8);
					//enemyNormalMap.Bind();
					//shaderProgram.setInt("normalMap3", 8);
					shaderProgram.setBool("isAsteroid", false);
					shaderProgram.setBool("isProjectile", false);
					shaderProgram.setBool("isFire", false);
					shaderProgram.setBool("isEnemy", true); // Set the isEnemy flag to true when drawing enemies

					glm::mat4 enemyTransform = glm::mat4(1.0f);
					enemyTransform = glm::translate(enemyTransform, enemy.position);

					// Apply rotation to face the spaceship
					enemyTransform = glm::rotate(enemyTransform, glm::radians(enemy.rotation), glm::vec3(0.0f, 0.0f, 1.0f));

					// Scale the enemy if needed, based on its texture size and desired appearance
					enemyTransform = glm::scale(enemyTransform, glm::vec3(enemy.radius)); // Use enemy.radius for scaling

					unsigned int enemyTransformLoc = glGetUniformLocation(shaderProgram.ID, "transform");
					glUniformMatrix4fv(enemyTransformLoc, 1, GL_FALSE, glm::value_ptr(enemyTransform));
					VAO1.Bind();
					glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
					shaderProgram.setBool("isEnemy", false); // Reset the flag after drawing
				}
			}

			// Enemy collision check:
			for (auto& enemy : enemies) {

				if (enemy.isBoss) {
					enemyandspaceshipcollisionradius = 1.0f;
				}
				else if (enemy.isBig) {
					enemyandspaceshipcollisionradius = 0.3f;
				}
				else {
					enemyandspaceshipcollisionradius = 0.23f;
				}
				if (enemy.active && glm::distance(enemy.position, spaceshipPosition) < (spaceshipCollisionRadius + enemy.radius) - enemyandspaceshipcollisionradius) {
					gameOver = true;
					enemy.active = false; // Deactivate enemy
					// Add explosion effect or decrement player health here
					break; // Exit the loop if a collision is detected
				}
			}

			// Update the logic to remove inactive enemies
			enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [](const Enemy& e) { return !e.active; }), enemies.end());

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
			
			
			// Draw asteroids
			for (auto& asteroid : asteroids) {

				shaderProgram.Activate();
				
				if (asteroid.isExploding) {
					std::cout << "Explosion frame: " << asteroid.currentExplosionFrame << std::endl;
					if (asteroid.currentExplosionFrame < numberOfExplosionFrames && explosionFrames[asteroid.currentExplosionFrame] != nullptr) {
						
						asteroid.update(deltaTime);

						int explosionTexUnit = 17 + asteroid.currentExplosionFrame;
						glActiveTexture(GL_TEXTURE0 + explosionTexUnit);
						if (asteroid.currentExplosionFrame >= 0 && asteroid.currentExplosionFrame < numberOfExplosionFrames) {
							if (explosionFrames[asteroid.currentExplosionFrame] &&
								glIsTexture(explosionFrames[asteroid.currentExplosionFrame]->ID)) {
								explosionFrames[asteroid.currentExplosionFrame]->Bind();
							}
						}
						else {
							std::cerr << "Error: Explosion frame out of range: " << asteroid.currentExplosionFrame << std::endl;
						}

						shaderProgram.setInt("currentExplosionFrame", asteroid.currentExplosionFrame);
						shaderProgram.setBool("isExploding", true);
					}
					else {
						std::cout << "Error: Invalid texture frame access." << std::endl;
					}
					
				}
				else {
					glActiveTexture(GL_TEXTURE1); // Activate the texture unit
					asteroidTexture.Bind();       // Bind the texture
					shaderProgram.setInt("tex1", 1);
					glActiveTexture(GL_TEXTURE2); // Activate the texture unit
					normalMap.Bind();             // Bind the texture
					shaderProgram.setInt("normalMap", 2);
					shaderProgram.setBool("isAsteroid", true);
					shaderProgram.setBool("isProjectile", false);
					shaderProgram.setBool("isEnemy", false);
					shaderProgram.setBool("isExploding", false);
				}
				// Similar to drawing the spaceship, create transformation matrices and draw each asteroid
				glm::mat4 asteroidTransform = glm::mat4(1.0f);
				asteroidTransform = glm::translate(asteroidTransform, asteroid.position);
				asteroidTransform = glm::scale(asteroidTransform, asteroid.scale);
				unsigned int asteroidTransformLoc = glGetUniformLocation(shaderProgram.ID, "transform");
				// Set transformation matrix in the shader and draw
				glUniformMatrix4fv(asteroidTransformLoc, 1, GL_FALSE, glm::value_ptr(asteroidTransform));
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
				//shaderProgram.setBool("isAsteroid", false);
				
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
	for (int i = 0; i < numberOfExplosionFrames; ++i) {
		if (explosionFrames[i] != nullptr) {
			delete explosionFrames[i];
			explosionFrames[i] = nullptr;
		}
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