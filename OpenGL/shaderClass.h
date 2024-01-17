#ifndef SHADER_CLASS_H
#define SHADER_CLASS_H

#include "glad.h"
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cerrno>
#include <glm/fwd.hpp>

std::string get_file_contents(const char* filename);

class Shader
{
public:
	// Reference ID of the Shader Program
	GLuint ID;
	// Constructor that build the Shader Program from 2 different shaders
	Shader(const char* vertexFile, const char* fragmentFile);

	// Activates the Shader Program
	void Activate();
	// Deletes the Shader Program
	void Delete();
	void setInt(const std::string& name, int value);
	void setBool(const std::string& name, bool value);
	void setMat4(const std::string& name, const glm::mat4& mat);
	void setVec2(const std::string& name, const glm::vec2& vec);
	void setVec3(const std::string& name, const glm::vec3& vec);
private:
	// Checks if the different Shaders have compiled properly
	void compileErrors(unsigned int shader, const char* type);
};


#endif
