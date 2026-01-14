#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTexCoord;

layout(binding = 0) readonly buffer Material
{
	vec3 color;	
} material;

layout (location = 0) out vec3 fragColor;
layout (location = 1) out vec3 fragNormal;
layout (location = 2) out vec3 fragTexCoord;

layout(push_constant) uniform PushConstants 
{
	mat4 model;
	mat4 view;
	mat4 projection;
} pc;

void main()
{
	gl_Position = pc.projection * pc.view * pc.model * vec4(inPosition, 1.0);
	fragColor = material.color;
}