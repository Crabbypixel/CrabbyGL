#ifdef SHADER_VERTEX

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNorm;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 matModel;
layout (std140)	uniform Matrices
{
	uniform mat4 matProjection;
	uniform mat4 matView;
};

out vec3 fNormal;
out vec3 vFragPos;
out vec2 TexCoords;

void main()
{
	gl_Position = matProjection * matView * matModel * vec4(vPos, 1.0f);

	vFragPos = vec3(matModel * vec4(vPos, 1.0f));
	fNormal = mat3(transpose(inverse(matModel))) * vNorm;
	TexCoords = aTexCoords;
}
#endif

#ifdef SHADER_FRAGMENT

struct Material
{
	sampler2D diffuse;
	sampler2D specular;

	float fShininess;
};

struct DirLight
{
	vec3 vDirection;
	vec3 vLightColor;

	vec3 vAmbient;
	vec3 vDiffuse;
	vec3 vSpecular;
};

struct PointLight
{
	vec3 vPosition;
	vec3 vLightColor;
	
	float fConstant;
	float fLinear;
	float fQuadratic;

	vec3 vAmbient;
	vec3 vDiffuse;
	vec3 vSpecular;
};

struct SpotLight
{
	vec3 vPosition;
	vec3 vDirection;
	vec3 vLightColor;

	vec3 vAmbient;
	vec3 vDiffuse;
	vec3 vSpecular;

	float fCutOff;
	float fOuterCutOff;

	float fConstant;
	float fLinear;
	float fQuadratic;
};

// Uniforms are indicated by the 'u_' prefix

#define	NR_POINT_LIGHTS 1
uniform DirLight u_dirLight;
uniform PointLight u_pointLights[NR_POINT_LIGHTS];
uniform SpotLight u_spotLight;

uniform vec3 u_vViewPos;
uniform Material u_material;

in vec3 fNormal;
in vec3 vFragPos;
in vec2 TexCoords;

out vec4 FragColor;

vec3 CalcDirLight(DirLight light, vec3 fNormal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 fNormal, vec3 vFragPos, vec3 vViewDir);
vec3 CalcSpotLight(SpotLight light, vec3 fNormal, vec3 vFragPos, vec3 vViewDir);

void main()
{
	vec3 vNorm = normalize(fNormal);
	vec3 vViewDir = normalize(u_vViewPos - vFragPos);

	vec3 vResult = vec3(0.0f, 0.0f, 0.0f);

	// Directional lighting
	vResult += CalcDirLight(u_dirLight, vNorm, vViewDir);

	// Point lighting
	for(int i=0; i<NR_POINT_LIGHTS; i++)
	{
		vResult += CalcPointLight(u_pointLights[i], vNorm, vFragPos, vViewDir);
	}

	// Spot lighting
	//vResult += CalcSpotLight(u_spotLight, vNorm, vFragPos, vViewDir);

	FragColor = vec4(vResult, 1.0f);
}

vec3 CalcSpotLight(SpotLight light, vec3 fNormal, vec3 vFragPos, vec3 vViewDir)
{
	vec3 vLightDir = normalize(light.vPosition - vFragPos);

	// Ambient shading
	vec3 vAmbient = light.vAmbient * light.vLightColor * texture(u_material.diffuse, TexCoords).rgb;

	// Diffuse shading
	float fDiff = max(dot(vLightDir, fNormal), 0.0f);
	vec3 vDiffuse = light.vDiffuse * light.vLightColor * fDiff * texture(u_material.diffuse, TexCoords).rgb;

	// Specular shading
	vec3 vReflectDir = reflect(-vLightDir, fNormal);
	float fSpec = pow(max(dot(vViewDir, vReflectDir), 0.0f), u_material.fShininess);
	vec3 vSpecular = light.vSpecular * light.vLightColor * fSpec * texture(u_material.specular, TexCoords).rgb;

	// SpotLight
	float fTheta = dot(vLightDir, normalize(-light.vDirection));
	float fEpsilion = light.fCutOff - light.fOuterCutOff;
	float fIntensity = clamp((fTheta - light.fOuterCutOff) / fEpsilion, 0.0f, 1.0f);
	vDiffuse = vDiffuse * fIntensity;
	vSpecular = vSpecular * fIntensity;

	// Attenuation
	float fDistance = length(light.vPosition - vFragPos);
	float fAttenuation = 1.0f / (light.fConstant + light.fLinear * fDistance + light.fQuadratic * (fDistance * fDistance));
	
	vAmbient = vAmbient * fAttenuation;
	vDiffuse = vDiffuse * fAttenuation;
	vSpecular = vSpecular * fAttenuation;

	return (vAmbient + vDiffuse + vSpecular);
}

vec3 CalcPointLight(PointLight light, vec3 fNormal, vec3 vFragPos, vec3 vViewDir)
{
	vec3 vLightDir = normalize(light.vPosition - vFragPos);

	// Ambient shading
	vec3 vAmbient = light.vAmbient * light.vLightColor * texture(u_material.diffuse, TexCoords).rgb;

	// Diffuse shading
	float fDiff = max(dot(fNormal, vLightDir), 0.0f);
	vec3 vDiffuse = light.vDiffuse * light.vLightColor * fDiff * texture(u_material.diffuse, TexCoords).rgb;

	// Specular shading
	vec3 vReflectDir = reflect(-vLightDir, fNormal);
	float fSpec = pow(max(dot(vViewDir, vReflectDir), 0.0f), u_material.fShininess);
	vec3 vSpecular = light.vSpecular * light.vLightColor * fSpec * texture(u_material.specular, TexCoords).rgb;

	// Attenuation
	float fDistance = length(light.vPosition - vFragPos);
	float fAttenuation = 1.0f / (light.fConstant + light.fLinear * fDistance + light.fQuadratic * (fDistance * fDistance));

	// We'll leave out attenuating the ambient shading
	vDiffuse = vDiffuse * fAttenuation;
	vSpecular = vSpecular * fAttenuation;

	return (vAmbient + vDiffuse + vSpecular);
}

vec3 CalcDirLight(DirLight light, vec3 fNormal, vec3 vViewDir)
{
	vec3 vLightDir = normalize(-light.vDirection);

	// Ambient shading
	vec3 vAmbient = light.vAmbient * light.vLightColor * texture(u_material.diffuse, TexCoords).rgb;

	// Diffuse shading
	float fDiff = max(dot(fNormal, vLightDir), 0.0f);
	vec3 vDiffuse = light.vDiffuse * light.vLightColor * fDiff * texture(u_material.diffuse, TexCoords).rgb;

	// Specular shading
	vec3 vReflectDir = reflect(-vLightDir, fNormal);
	float fSpec = pow(max(dot(vViewDir, vReflectDir), 0.0f), u_material.fShininess);
	vec3 vSpecular = light.vSpecular * light.vLightColor * fSpec * texture(u_material.specular, TexCoords).rgb;

	return (vAmbient + vDiffuse + vSpecular);
}

#endif