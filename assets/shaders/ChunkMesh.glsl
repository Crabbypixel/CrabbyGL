#ifdef SHADER_VERTEX
layout (location = 0) in vec3  aPos;
layout (location = 1) in vec2  aUV;
layout (location = 2) in vec2  aUVTileMin;
layout (location = 3) in vec2  aUVTileMax;
layout (location = 4) in vec2  aOverlayUV;
layout (location = 5) in vec3  aNormal;
layout (location = 6) in vec3  aTint;
layout (location = 7) in float aUseOverlay;
layout (location = 8) in float aAo;

// Shared projection + view matrices via UBO
layout (std140) uniform Matrices {
    mat4 matProjection;
    mat4 matView;
};

out vec3  fWorldPos;
out vec2  fUV;
out vec2  fUVTileMin;
out vec2  fUVTileMax;
out vec2  fOverlayUV;
out vec3  fNormal;
out vec3  fTint;
out float fUseOverlay;
out float fAo;

void main()
{
    fWorldPos    = aPos;
    fUV          = aUV;
    fUVTileMin   = aUVTileMin;
    fUVTileMax   = aUVTileMax;
    fOverlayUV   = aOverlayUV;
    fNormal      = aNormal;
    fTint        = aTint;
    fUseOverlay  = aUseOverlay;
    fAo          = aAo;

    gl_Position = matProjection * matView * vec4(aPos, 1.0f);
}
#endif

#ifdef SHADER_FRAGMENT

uniform sampler2D u_atlas;

uniform vec3 u_lightDir;
uniform vec3 u_ambient;
uniform vec3 u_diffuse;

// Selection highlight
uniform bool  u_isSelected;
uniform ivec3 u_selectedBlock;

// Feature flags
uniform bool u_isAOEnabled;

in vec3  fWorldPos;
in vec2  fUV;
in vec2  fUVTileMin;
in vec2  fUVTileMax;
in vec2  fOverlayUV;
in vec3  fNormal;
in vec3  fTint;
in float fUseOverlay;
in float fAo;

out vec4 FragColor;

// --- Tile-local UV -> atlas UV -------------------------------------
//
// v_uv goes 0..N for an N-wide merged quad
// fract() wraps back to [0, 1), then we scale into the 16x16 tile region
//
// N-wide quad -> uv.x goes 0 to N across the surface
// fract(0.0)=0, fract(0.5)=0.5, fract(1.0)=0.0 (GL spec), fract(1.5)=0.5

vec2 tileUV(vec2 tileLocal, vec2 tileMin, vec2 tileMax)
{
    return tileMin + fract(tileLocal) * (tileMax - tileMin);
}

ivec3 fragBlockPos()
{
    return ivec3(floor(fWorldPos - fNormal * 0.01f));
}

void main()
{
    // --- Base texture -------------------------------------
     vec2 atlasUV = tileUV(fUV, fUVTileMin, fUVTileMax);

    // Correct mipmap derivatives — from smooth tile-local
    vec2 tileSize = fUVTileMax - fUVTileMin;
    vec2 dx = dFdx(fUV) * tileSize;
    vec2 dy = dFdy(fUV) * tileSize;

    vec4 baseTex = textureGrad(u_atlas, atlasUV, dx, dy);

    if (baseTex.a < 0.1)
        discard;

    vec3 color = baseTex.rgb;
    
    if (fUseOverlay > 0.5f) {
        vec4 overlay = textureGrad(u_atlas, fOverlayUV, dFdx(fOverlayUV), dFdy(fOverlayUV));
        color = mix(color, overlay.rgb * fTint, overlay.a);
    } else {
        color *= fTint;
    }

    // --- Diffuse lighting (flat per-face) -------------------------------------
    // Voxels use flat face normals — no normal map, no per-fragment interpolation
    float NdotL = max(dot(normalize(fNormal), -normalize(u_lightDir)), 0.0f);
    color *= u_ambient + u_diffuse * NdotL;

    // --- Ambient occlusion -------------------------------------
    if (u_isAOEnabled)
        color *= mix(0.5f, 1.0f, fAo);

    // Selection highlight 
    if(u_isSelected && fragBlockPos() == u_selectedBlock)
        color.rgb /= 0.85f;

    FragColor = vec4(color, baseTex.a);
}
#endif