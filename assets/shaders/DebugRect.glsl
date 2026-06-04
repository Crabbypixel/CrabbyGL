#ifdef SHADER_VERTEX
layout (location = 0) in vec2 aPos;

uniform mat4 matProjection;

void main()
{
	gl_Position = matProjection * vec4(aPos, 0.0f, 1.0f);
}
#endif

#ifdef SHADER_FRAGMENT
out vec4 FragColor;
uniform vec4 uColor;

void main()
{
	FragColor = uColor;
}
#endif