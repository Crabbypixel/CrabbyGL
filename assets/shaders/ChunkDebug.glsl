#ifdef SHADER_VERTEX
layout(location = 0) in vec3 aPos;

layout(std140) uniform Matrices
{
    mat4 projection;
    mat4 view;
};

void main()
{
    gl_Position = projection * view * vec4(aPos, 1.0f);
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