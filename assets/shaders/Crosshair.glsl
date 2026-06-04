#ifdef SHADER_VERTEX
layout (location  = 0) in vec2 aPos;
uniform float uAspect;

void main()
{
    vec2 pos = aPos;
	pos.y *= uAspect;
	gl_Position = vec4(pos, 0.0f, 1.0f);
}
#endif

#ifdef SHADER_FRAGMENT
out vec4 FragColor;

void main()
{
	FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
}
#endif