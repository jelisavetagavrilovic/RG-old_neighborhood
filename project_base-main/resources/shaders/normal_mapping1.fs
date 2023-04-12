#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 TangentLightPos;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
} fs_in;

#define NUM_LIGHTS 6
struct LightPos {
    vec3 direction[NUM_LIGHTS];
};

uniform sampler2D diffuseMap;
uniform sampler2D normalMap;
uniform LightPos lightPos;
uniform vec3 viewPos;

void main()
{
     // obtain normal from normal map in range [0,1]
    vec3 normal = texture(normalMap, fs_in.TexCoords).rgb;
    // transform normal vector to range [-1,1]
    normal = normalize(normal * 2.0 - 1.0);  // this normal is in tangent space

    vec3 result = vec3(0.0);
    for(int i = 0; i < NUM_LIGHTS; i++)
    {
        // get diffuse color
        vec3 color = texture(diffuseMap, fs_in.TexCoords).rgb;
        // ambient
        vec3 ambient = 0.1 * color;
        // diffuse
        vec3 lightDir = normalize(fs_in.TangentLightPos - fs_in.TangentFragPos);
        float diff = max(dot(lightDir, normal), 0.0);
        vec3 diffuse = diff * color;
        // specular
        vec3 viewDir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
        vec3 reflectDir = reflect(-lightDir, normal);
        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);

        vec3 specular = vec3(0.2) * spec;
        result += vec3(ambient + diffuse + specular);
    }

    FragColor = vec4(result, 1.0);
}