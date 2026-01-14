#version 450

struct Material
{
    vec3 color;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTexCoord;

layout(location = 0) out vec3 fragColor;

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
    float outlineWidth = 0.03;
    vec3 expandedPosition = inPosition + normalize(inNormal) * outlineWidth;
    
    gl_Position = vp.projection * vp.view * pushConstants.model * vec4(expandedPosition, 1.0);
    fragColor = vec3(1.0, 0.647, 0.0);
}