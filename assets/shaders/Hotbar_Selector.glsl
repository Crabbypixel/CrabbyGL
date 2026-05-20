#ifdef SHADER_VERTEX
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 fTexCoord;
uniform mat4 matProjection;

void main()
{
	fTexCoord = aTexCoord;

    gl_Position = matProjection * vec4(aPos, 1.0);
}
#endif

#ifdef SHADER_FRAGMENT
out vec4 FragColor;

in vec2 fTexCoord;

uniform sampler2D hotbarSelectorTexture;

void main()
{
	FragColor = texture(hotbarSelectorTexture, fTexCoord);

	if(FragColor.a < 0.1f) {
		discard;
	}
}
#endif