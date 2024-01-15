#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal; // Assuming normals are at location 1
layout (location = 2) in vec2 aTex;

out vec2 texCoord;
out vec3 FragPos;
out vec3 Normal; // Pass normal to the fragment shader


uniform mat4 model;
uniform mat4 transform;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * transform * vec4(aPos, 1.0);
    texCoord = aTex;
    FragPos = vec3(model * vec4(aPos, 1.0)); // Calculate world position of the vertex
    Normal = mat3(transpose(inverse(model))) * aNormal; // Correctly transform the normal
}