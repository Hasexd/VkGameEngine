#version 450

struct Material
{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shininess;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout (location = 0) out vec3 fragColor;
layout (location = 1) out vec3 fragNormal;
layout (location = 2) out vec2 fragTexCoord;

layout(binding = 0) readonly buffer VP
{
	mat4 view;
	mat4 projection;
} vp;

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
	gl_Position = vp.projection * vp.view * pushConstants.model * vec4(inPosition, 1.0);
	
	mat3 normalMatrix = mat3(transpose(inverse(pushConstants.model)));
	fragNormal = normalMatrix * inNormal;
	
	if(pushConstants.materialIndex == -1)
	{
		fragColor = vec3(0.5, 0.5, 0.5);
	}
	else
	{
		fragColor = materials[pushConstants.materialIndex].diffuse;
	}
	
	fragTexCoord = inTexCoord;
}