#version 330 core

out vec4 colour;

uniform vec3 lightColour; 

void main()
{
    colour = vec4(lightColour,1.0f) ; 
}