#version 330 core

// Inputs from the vertex shader
in vec2 texCoord;

// Output color of the pixel
out vec4 FragColor;

// Texture sampler for texture unit 0
uniform sampler2D tex0;

void main()
{
    // Sample the texture with the given texture coordinates.
    // No multiplication with vertex color, so the texture appears as is.
    FragColor = texture(tex0, texCoord);
}
