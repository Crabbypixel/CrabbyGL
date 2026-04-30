#ifdef SHADER_VERTEX

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNorm;

out vec3 Normal;
out vec3 FragPos;

uniform mat4 matModel;
layout (std140) uniform Matrices
{
	uniform mat4 matProjection;
	uniform mat4 matView;
};

void main()
{
	gl_Position = matProjection * matView * matModel * vec4(aPos, 1.0f);
	
	Normal = mat3(inverse(transpose(matModel))) * aNorm;
	FragPos = vec3(matModel * vec4(aPos, 1.0f));
}

#endif


#ifdef SHADER_FRAGMENT

struct PointLight
{
	vec3 position;
	vec3 color;

	float fConstant;
	float fLinear;
	float fQuadratic;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};
#define NR_POINT_LIGHTS 1
uniform PointLight pointlights[NR_POINT_LIGHTS];

vec3 CalcPointLight(PointLight light, vec3 Normal, vec3 FragPos, vec3 viewDir);

in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;

uniform vec3 materialColor;
uniform vec3 viewPos;

void main()
{
	// For further calculations
	vec3 norm = normalize(Normal);
	vec3 viewDir = normalize(viewPos - FragPos);

	vec3 vFinal = vec3(0.0f, 0.0f, 0.0f);

	// Point lights
	vFinal += CalcPointLight(pointlights[0], norm, FragPos, viewDir);

	FragColor = vec4(vFinal, 1.0f);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
	vec3 lightDir = normalize(light.position - FragPos);

	// Ambient shading
	vec3 ambient = light.ambient * light.color * materialColor;

	// Diffuse shading
	float fDiffuse = max(dot(normal, lightDir), 0.0f);
	vec3 diffuse = light.diffuse * light.color * fDiffuse * materialColor;

	// Specular shading
	vec3 reflectDir = reflect(-lightDir, normal);
	float fSpecular = pow(max(dot(viewDir, reflectDir), 0.0f), 128);
	vec3 specular = light.specular * light.color * fSpecular * materialColor;

	return (ambient + diffuse + specular);
}

#endif