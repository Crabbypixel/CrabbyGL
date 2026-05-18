#ifdef SHADER_VERTEX
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;

out vec3 fColor;
out vec2 fTexCoord;
uniform mat4 matProjection;

void main()
{
    fColor = aColor;
    fTexCoord = aTexCoord;

    gl_Position = matProjection * vec4(aPos, 1.0);
}
#endif

#ifdef SHADER_FRAGMENT
out vec4 FragColor;

in vec3 fColor;
in vec2 fTexCoord;

uniform sampler2D hotbarTexture;

void main()
{
    FragColor = texture(hotbarTexture, fTexCoord);
}
#endif