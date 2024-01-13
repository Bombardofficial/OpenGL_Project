// Fragment Shader Code
#version 330 core

in vec2 texCoord;
in vec3 Normal;

out vec4 FragColor;

uniform sampler2D tex0; // Texture unit for the spaceship texture
uniform sampler2D tex1; // Texture unit for the asteroid texture
uniform sampler2D normalMap; // Normal map for the asteroid

uniform vec3 lightDir; // Direction to the light source
uniform float lightIntensity; // Light intensity
uniform bool isAsteroid; // Flag to indicate if the current object is an asteroid

void main() {
    vec3 norm = normalize(Normal);
    vec3 lightDirection = normalize(lightDir);

    vec4 texColor = texture(tex0, texCoord); // Fetch the spaceship texture color (with alpha)
    vec3 materialColor = vec3(1.0, 1.0, 1.0); // Base reflectivity for the spaceship

    if (isAsteroid) {
        vec3 texNormal = texture(normalMap, texCoord).rgb;
        texNormal = normalize(texNormal * 2.0 - 1.0);
        norm = texNormal;
        texColor = texture(tex1, texCoord); // Fetch the asteroid texture color (with alpha)
        materialColor = texColor.rgb; // Material color is taken from the asteroid's texture
    }

    // Calculate the diffuse lighting component
    float diff = max(dot(norm, lightDirection), 0.0);
    vec3 diffuse = diff * lightIntensity * materialColor; // Multiply light intensity by base reflectivity

    // Add ambient lighting
    vec3 ambient = 0.1 * materialColor; // Ambient light is based on base reflectivity

    vec3 result = ambient + diffuse;
    result *= texColor.rgb; // Modulate the result by the texture color to include texture details

    // Set the final color of the pixel with alpha
    FragColor = vec4(result, texColor.a); // Use the alpha from the texture
}
