#version 330 core

out vec4 colour;

in vec4 vCol ;
in vec2 TexCoord;
in vec3 Normal; 
in vec3 FragPos;
in vec3 VertexColour;
in vec4 FragPosLightSpace;

uniform vec3 lightColour; 
uniform vec3 lightPos;

uniform vec3 viewPos ;

uniform sampler2D texture_data;
uniform bool useTexture; // false = use the material colour from the .obj/.mtl
uniform vec3 tintColour;  // multiplies the base colour (e.g. paint a white cube red)

// World-space UV: uv = (worldPos - texOffset) * texScale
// texScale = repeats per metre (x: across, y: up/along), texOffset = where uv (0,0) is
uniform bool worldUV;
uniform vec2 texScale;
uniform vec2 texOffset;

uniform float specularStrength;

uniform sampler2D shadowMap;


// Normal facing the camera side of the triangle.
// Some parts of character.obj (e.g. the robot's face) were exported with normals
// pointing inward; when we see the back of a triangle, flip its normal so it is lit
// like the outside. Correct meshes are unaffected (we only see their front faces).
vec3 SurfaceNormal() {
    vec3 n = normalize(Normal);
    return gl_FrontFacing ? n : -n;
}

vec3 ambientLight() { 
    float ambientStrength = 0.3f ; 
    vec3 ambient = lightColour * ambientStrength;
    return ambient ; 
}

vec3 diffuseLight() { 
    float diffuseStrength =  0.5 ; 
    vec3 norm = SurfaceNormal() ;
    vec3 lightDir = normalize(lightPos - FragPos);

    float diff = max(dot(norm , lightDir),0.0f); 
    vec3 diffuse = lightColour * diff * diffuseStrength ; 

    return diffuse ; 
} 

vec3 specularlight() { 
    float shininess = 256.0f ; 

    vec3 viewDir = normalize(viewPos - FragPos) ; 
    vec3 norm = SurfaceNormal() ;
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 reflectDir = reflect(-lightDir , norm) ;
    //Phong
    // float spec = pow(max(dot(reflectDir , viewDir), 0.0f), shininess);
    
    //Blinn-Phong 
    vec3 halfDir = normalize((lightDir + viewDir)/2.0f);
    float spec = pow(max(dot(halfDir, norm), 0.0f), shininess);


    vec3 specular = lightColour * spec * specularStrength ; 
    
    return specular ;
}

float ShadowCalculation()
{
    vec3 projCoords = FragPosLightSpace.xyz / FragPosLightSpace.w ;
    projCoords = projCoords * 0.5 + 0.5 ;

    // Outside the light's ortho box = not in shadow
    if (projCoords.z > 1.0) return 0.0;

    float currentDepth = projCoords.z;

    // Slope-scaled bias: surfaces facing away from the light need more bias (less acne)
    vec3 norm = SurfaceNormal();
    vec3 lightDir = normalize(lightPos - FragPos);
    float bias = max(0.003 * (1.0 - dot(norm, lightDir)), 0.0008);

    // PCF 3x3 (soft edges)
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    return shadow / 9.0;
}

vec2 WorldUV()
{
    // Pick the plane the face lies in from its normal (top -> xz, front -> xy, side -> zy)
    vec3 n = abs(normalize(Normal));
    if (n.y >= n.x && n.y >= n.z) return (FragPos.xz - texOffset) * texScale;
    if (n.z >= n.x)               return (FragPos.xy - texOffset) * texScale;
    return (FragPos.zy - texOffset) * texScale;
}

void main()
{
    vec2 uv = worldUV ? WorldUV() : TexCoord;
    vec4 baseColour = useTexture ? texture(texture_data, uv) : vec4(VertexColour, 1.0);
    baseColour.rgb *= tintColour;

    float shadow = ShadowCalculation();
    vec3 lighting = ambientLight() + (1.0 - shadow) * (diffuseLight() + specularlight());
    colour = baseColour * vec4(lighting, 1.0);
}