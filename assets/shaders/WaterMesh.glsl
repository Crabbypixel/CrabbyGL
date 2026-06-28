#ifdef SHADER_VERTEX

// Vertex attributes — same layout as full shader, unchanged
layout (location = 0) in vec3  aPos;
layout (location = 1) in vec2  aUV;
layout (location = 2) in uint  aTileBase;
layout (location = 3) in uint  aTileOverlay;
layout (location = 4) in uint  aPacked;
layout (location = 5) in uint  aLightValue;
layout (location = 6) in vec4  aTint;

layout (std140) uniform Matrices
{
    mat4 matProjection;
    mat4 matView;
};

out vec2  fUV;
out vec3  fTint;
out float fAlpha;

flat out uint fTileBase;

void main()
{
    fUV       = aUV;
    fTint     = aTint.rgb;
    fAlpha    = aTint.a;
    fTileBase = aTileBase;

    gl_Position = matProjection * matView * vec4(aPos, 1.0f);
}

#endif

#ifdef SHADER_FRAGMENT

uniform sampler2D u_atlas;

in vec2  fUV;
in vec3  fTint;
in float fAlpha;

flat in uint fTileBase;

out vec4 FragColor;

vec2 tileUV(vec2 tileLocal, vec2 tileMin, vec2 tileMax)
{
    return tileMin + fract(tileLocal) * (tileMax - tileMin);
}

void main()
{
    const float TILE_W = 0.0625f;
    const float TILE_H = 0.0625f;

    uint col = fTileBase % 16u;
    uint row = fTileBase / 16u;

    vec2 tileMin = vec2(float(col) * TILE_W, 1.0f - (float(row) + 1.0f) * TILE_H);
    vec2 tileMax = tileMin + vec2(TILE_W, TILE_H);

    vec2 atlasUV = tileUV(fUV, tileMin, tileMax);
    vec4 baseTex = texture(u_atlas, atlasUV);

    vec3 color = baseTex.rgb * fTint;

    // No AO. No overlay. No directional light. No torch/sky. No selection highlight.
    // No textureGrad/mip handling — plain texture() sample only.
    // If z-fight wedges still appear with THIS shader, it's 100% geometry/depth,
    // not shading — narrows the bug to vertex data or GL state, not GLSL logic.

    FragColor = vec4(color, baseTex.a);
}

#endif
