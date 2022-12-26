#version 330

in vec3 FragPos;
in vec3 Normal;

uniform vec3 LightPos;
uniform sampler2D texture0;

out vec4 finalColor;

void main()
{
    // Calculate the diffuse lighting intensity
    vec3 lightDir = normalize(LightPos - FragPos);
    float diff = max(dot(Normal, lightDir), 0.0);

    // Calculate the specular lighting intensity
    vec3 viewDir = normalize(-FragPos);
    vec3 reflectDir = reflect(-lightDir, Normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);

    // Combine the diffuse and specular lighting intensities with the texture color
    vec4 texColor = texture(texture0, vec2(FragPos.x, FragPos.y));
    FragColor = vec4(diff * texColor.rgb + spec * vec3(1, 1, 1), texColor.a);
}