#version 450

struct Material
{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shininess;
};

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec3 fragNormal;
layout (location = 2) in vec2 fragTexCoord;

layout (location = 0) out vec4 outColor;

layout (binding = 1) readonly buffer MaterialBuffer
{
	Material[] materials;
};

layout(push_constant) uniform PushConstants 
{
	mat4 model;
	uint materialIndex;
} pushConstants;

void main()
{
	vec3 lightDir = normalize(vec3(0.5, -1.0, 0.3));
	vec3 lightColor = vec3(1.0, 1.0, 1.0);
	
	vec3 norm = normalize(fragNormal);
	
	vec3 ambient = fragColor * 0.3;
	
	float diff = max(dot(norm, -lightDir), 0.0);
	vec3 diffuse = diff * fragColor * lightColor;
	
	vec3 result = ambient + diffuse;
	
	outColor = vec4(result, 1.0);
}