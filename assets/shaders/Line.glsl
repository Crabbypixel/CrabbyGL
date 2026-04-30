#ifdef SHADER_VERTEX

layout (location = 0) in vec3 vPos;

uniform mat4 matModel;
layout (std140)	uniform Matrices
{
	uniform mat4 matProjection;
	uniform mat4 matView;
};

void main()
{
	gl_Position = matProjection * matView * matModel * vec4(vPos, 1.0f);
}
#endif


#ifdef SHADER_FRAGMENT

uniform vec3 vColor;
out vec4 FragColor;

void main()
{
	FragColor = vec4(vColor, 1.0f);
}
#endif