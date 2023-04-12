#version 330 core
out vec4 FragColor;

#define NUM_LIGHTS 6
in VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 TangentLightPos[NUM_LIGHTS];
    vec3 TangentViewPos[NUM_LIGHTS];
    vec3 TangentFragPos[NUM_LIGHTS];
} fs_in;

struct LightsPos {
    vec3 direction[NUM_LIGHTS];
};

struct DirLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;

    vec3 specular;
    vec3 diffuse;
    vec3 ambient;

    float constant;
    float linear;
    float quadratic;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform sampler2D diffuseMap;
uniform sampler2D normalMap;
uniform sampler2D depthMap;
uniform DirLight dirLight;
uniform PointLight pointLight[NUM_LIGHTS];
uniform SpotLight spotLight[NUM_LIGHTS];
uniform LightsPos lightPos;
uniform vec3 viewPos;
uniform float heightScale;


vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{
    float height =  texture(depthMap, texCoords).r;
    return texCoords - viewDir.xy * (height * heightScale);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 viewDir, int i, vec2 texCoords)
{
    // diffuse
    vec3 lightDir = normalize(fs_in.TangentLightPos[i] - fs_in.TangentFragPos[i]);
    float diff = max(dot(lightDir, normal), 0.0);

    // specular shading
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);

    // attenuation
    float distance = length(light.position - fs_in.TangentFragPos[i]);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    // vec3 color = texture(diffuseMap, fs_in.TexCoords).rgb;
    vec3 color = texture(diffuseMap, texCoords).rgb;

    // combine results
    vec3 ambient = light.ambient * color;
    vec3 diffuse = light.diffuse * diff * color;
    vec3 specular = light.specular * spec * color;
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 viewDir, int i, vec2 texCoords)
 {
     vec3 lightDir = normalize(fs_in.TangentLightPos[i] - fs_in.TangentFragPos[i]);
     // diffuse shading
     float diff = max(dot(normal, lightDir), 0.0);
     // specular shading
     vec3 halfwayDir = normalize(lightDir + viewDir);
     float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
     // attenuation
     float distance = length(light.position - fs_in.TangentFragPos[i]);
     float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
     // spotlight intensity
     float theta = dot(lightDir, normalize(-light.direction));
     float epsilon = light.cutOff - light.outerCutOff;
     float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
     // vec3 texColor = vec3(texture(diffuseMap, fs_in.TexCoords));
     vec3 texColor = texture(diffuseMap, texCoords).rgb;

     // combine results
     vec3 ambient = light.ambient * texColor;
     vec3 diffuse = light.diffuse * diff * texColor;
     vec3 specular = light.specular * spec * texColor;
     ambient *= attenuation * intensity;
     diffuse *= attenuation * intensity;
     specular *= attenuation * intensity;
     return (ambient + diffuse + specular);
 }


vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec2 texCoords)
{
    vec3 lightDir = normalize(-light.direction);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);

    // specular shading
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);

    // get diffuse color
    // vec3 color = texture(diffuseMap, fs_in.TexCoords).rgb;
    vec3 color = texture(diffuseMap, texCoords).rgb;
    // ambient
    vec3 ambient = light.ambient * color;
    vec3 diffuse = light.diffuse * diff * color;
    vec3 specular = light.specular * spec * color;

    return (ambient + diffuse + specular);
}

void main()
{
     // obtain normal from normal map in range [0,1]
    vec3 normal = texture(normalMap, fs_in.TexCoords).rgb;
    // transform normal vector to range [-1,1]
    normal = normalize(normal * 2.0 - 1.0);  // this normal is in tangent space
    // result
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);


    vec2 texCoords = fs_in.TexCoords;
    texCoords = ParallaxMapping(fs_in.TexCoords,  viewDir);
    if(texCoords.x > 7.5 || texCoords.y > 7.5 || texCoords.x < 0.0 || texCoords.y < 0.0)
        discard;

    vec3 result = CalcDirLight(dirLight, normal, viewDir, texCoords);
    for(int i = 0; i < NUM_LIGHTS; i++)
    {
        viewDir = normalize(fs_in.TangentViewPos[i] - fs_in.TangentFragPos[i]);

        result += CalcPointLight(pointLight[i], normal, viewDir, i, texCoords);
        result +=  CalcSpotLight(spotLight[i], normal, viewDir, i, texCoords);
    }

    FragColor = vec4(result, 1.0);
}