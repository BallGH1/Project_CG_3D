#version 330 core

out vec4 colour;

uniform vec3 bgColour; 
in vec4 FragPosLightSpace;

uniform sampler2D shadowMap;

float ShadowCalculation()
{  
    vec3 projCoords = FragPosLightSpace.xyz / FragPosLightSpace.w ;
    projCoords = projCoords * 0.5 + 0.5 ; 
    
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    float bias = 0.005 ;

    // float shadow = currentDepth - bias > closestDepth? 1.0:0.0; 
    float shadow = 0.0; 
    vec2 textlSize = 1.0 / textureSize(shadowMap, 0);
      for(int x = -1; x <= 1 ; ++x) {  
        for(int y = -1; y <=1 ; y++) {  
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x,y) * textlSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0; 
        }
    }
    shadow /= 9.0;
    return shadow ; 
}

void main()
{
    float shadow = ShadowCalculation() ; 
    colour = vec4((1.0f - shadow) * bgColour,1.0f) ; 
}