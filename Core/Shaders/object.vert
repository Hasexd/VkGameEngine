#version 450

struct Material
{
	vec3 color;
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

layout(push_constant) uniform PushConstants 
{
	mat4 model;
	Material material;
} pushConstants;


void main()
{
	gl_Position = vp.projection * vp.view * pushConstants.model * vec4(inPosition, 1.0);
	fragColor = pushConstants.material.color;
}