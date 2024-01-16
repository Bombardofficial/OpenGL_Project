#version 330 core

in vec2 texCoord;
in vec3 Normal;

out vec4 FragColor;

uniform sampler2D tex0; // Texture unit for the spaceship texture
uniform sampler2D tex1; // Texture unit for the asteroid texture
uniform sampler2D tex2; // Texture unit for the projectile texture
uniform sampler2D tex3; // Texture unit for the fire1 texture
uniform sampler2D tex4; // Texture unit for the fire2 texture
uniform sampler2D tex5; // Texture unit for the fire3 texture
uniform sampler2D tex6; // Texture unit for the fire4 texture
uniform sampler2D tex7; // Texture unit for the enemy texture
uniform sampler2D normalMap; // Normal map for the asteroid
uniform sampler2D normalMap2; // Normal map for the spaceship
uniform sampler2D normalMap3; // Normal map for the enemy
uniform vec3 lightDir; // Direction to the light source
uniform float lightIntensity; // Light intensity
uniform bool isAsteroid; // Flag to indicate if the current object is an asteroid
uniform bool isProjectile; // Flag to indicate if the current object is a projectile
uniform bool isFire; // Flag to indicate if the current object is fire
uniform sampler2D fireTextures[4];
uniform int currentFireFrame; // Uniform declaration in the fragment shader
uniform bool isEnemy; // Flag to indicate if the current object is an enemy

void main() {
    vec3 norm = normalize(Normal);
    vec3 lightDirection = normalize(lightDir);

    vec4 texColor;
    vec3 materialColor = vec3(1.0, 1.0, 1.0); // Default reflectivity

    if (isAsteroid) {
        vec3 texNormal = texture(normalMap, texCoord).rgb;
        texNormal = normalize(texNormal * 2.0 - 1.0);
        norm = texNormal;
        texColor = texture(tex1, texCoord); // Fetch the asteroid texture color
        materialColor = texColor.rgb;
    } else if (isProjectile) {
        texColor = texture(tex2, texCoord); // Fetch the projectile texture color
        vec3 brightColor = texColor.rgb; // Use the RGB color from the texture
        FragColor = vec4(brightColor, texColor.a); // Set the final color with full brightness
        return; // Skip the rest of the lighting calculations
    } else if (isFire) {
        texColor = texture(fireTextures[currentFireFrame], texCoord);
        FragColor = vec4(texColor.rgb, texColor.a);
        return;
    } else if (isEnemy) {
        vec3 texNormal = texture(normalMap3, texCoord).rgb;
		texNormal = normalize(texNormal * 2.0 - 1.0);
		norm = texNormal;
		texColor = texture(tex7, texCoord); // Fetch the enemy texture color
		materialColor = texColor.rgb;
    }else {
        // Assume it's the spaceship
        vec3 texNormal = texture(normalMap2, texCoord).rgb;
        texNormal = normalize(texNormal * 2.0 - 1.0);
        norm = texNormal;
        texColor = texture(tex0, texCoord); // Fetch the spaceship texture color
        materialColor = texColor.rgb;
    }

    // Calculate the diffuse lighting component
    float diff = max(dot(norm, lightDirection), 0.0);
    vec3 diffuse = diff * lightIntensity * materialColor;

    // Add ambient lighting
    vec3 ambient = 0.1 * materialColor;

    vec3 result = ambient + diffuse;
    result *= texColor.rgb;

    FragColor = vec4(result, texColor.a); // Use the alpha from the texture
}
