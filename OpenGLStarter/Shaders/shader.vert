#version 330 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 aTexCoord; 
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec3 aColour; // material colour from .mtl


out vec4 vCol;
out vec2 TexCoord;
out vec3 Normal ; 
out vec3 FragPos ; 
out vec3 VertexColour;
out vec4 FragPosLightSpace; // position seen from the light (for the shadow map)

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform mat4 lightView;
uniform mat4 lightProjection;

void main()
{
    gl_Position = projection * view * model * vec4(pos, 1.0f);
    FragPos = vec3(model * vec4(pos, 1.0f)) ;
    vCol = vec4(clamp(pos, 0.0f, 1.0f), 1.0f);
    TexCoord = aTexCoord;
    // Normal matrix: keeps normals correct after rotate/scale in model
    Normal = mat3(transpose(inverse(model))) * aNormal ; 
    VertexColour = aColour;
    FragPosLightSpace = lightProjection * lightView * model * vec4(pos, 1.0f);
}