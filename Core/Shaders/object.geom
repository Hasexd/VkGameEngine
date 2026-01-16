#version 450

layout(triangles) in;
layout(line_strip, max_vertices = 4) out;

layout(location = 0) in vec3 fragColor[];
layout(location = 1) in vec3 fragNormal[];
layout(location = 2) in vec2 fragTexCoord[];

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outTexCoord;

void main() 
{
    for(int i = 0; i < 3; i++) 
    {
        gl_Position = gl_in[i].gl_Position;
        outColor = fragColor[i];
        EmitVertex();
    }
    
    gl_Position = gl_in[0].gl_Position;
    outColor = fragColor[0];
    EmitVertex();
    
    EndPrimitive();
}