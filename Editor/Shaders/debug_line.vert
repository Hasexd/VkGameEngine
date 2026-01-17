#version 450

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 fragColor;

layout(binding = 0) readonly buffer VP
{
	mat4 view;
	mat4 projection;
} vp;

layout(push_constant) uniform PushConstants
{
	mat4 model;
	vec3 color;
} pushConstants;

void main()
{
	gl_Position = vp.projection * vp.view * pushConstants.model * vec4(inPosition, 1.0);
	fragColor = pushConstants.color;
}