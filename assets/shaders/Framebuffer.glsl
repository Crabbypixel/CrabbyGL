#ifdef SHADER_VERTEX

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main()
{
	// We are directly passing NDC here, no fancy matrix stuffs here!
	gl_Position = vec4(aPos.x, aPos.y, 0.0f, 1.0f);
	TexCoords = aTexCoords;
}

#endif

#ifdef SHADER_FRAGMENT

out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture;

void main()
{
	FragColor = texture(screenTexture, TexCoords);
}

#endif