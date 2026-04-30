#ifdef SHADER_VERTEX

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 matModel;
layout (std140)	uniform Matrices
{
	uniform mat4 matProjection;
	uniform mat4 matView;
};
out vec2 TexCoords;

void main()
{
	TexCoords = aTexCoords;
	gl_Position = matProjection * matView * matModel * vec4(aPos, 1.0f);
}

#endif

#ifdef SHADER_FRAGMENT

in vec2 TexCoords;
out vec4 FragColor;
uniform vec3 lightColor;
//uniform sampler2D texturemap;

void main()
{
	//vec3 color = texture(texturemap, TexCoords).rgb;
	//FragColor = vec4(color, 1.0f);
	FragColor = vec4(lightColor, 1.0f);
}

#endif