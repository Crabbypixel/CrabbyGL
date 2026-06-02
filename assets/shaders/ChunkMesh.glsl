#ifdef SHADER_VERTEX
layout (location = 0) in vec3  aPos;
layout (location = 1) in vec2  aUV;
layout (location = 2) in uint  aTileBase;
layout (location = 3) in uint  aTileOverlay;
layout (location = 4) in uint  aNormalIndex;
layout (location = 5) in float aUseOverlay;
layout (location = 6) in float aAo;
layout (location = 7) in vec3  aTint;

// Shared projection + view matrices via UBO
layout (std140) uniform Matrices {
    mat4 matProjection;
    mat4 matView;
};

out vec3  fWorldPos;
out vec2  fUV;
out vec3  fTint;
out float fUseOverlay;
out float fAo;
flat out uint fTileBase;
flat out uint fTileOverlay;
flat out uint fNormalIndex;

void main()
{
    fWorldPos    = aPos;
    fUV          = aUV;
    fTileBase    = aTileBase;
    fTileOverlay = aTileOverlay;
    fNormalIndex      = aNormalIndex;
    fTint        = aTint;
    fUseOverlay  = aUseOverlay;
    fAo          = aAo;

    gl_Position = matProjection * matView * vec4(aPos, 1.0f);
}
#endif

#ifdef SHADER_FRAGMENT
struct UVRect
{
    vec2 _min;
    vec2 _max;
};

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
in vec3  fTint;
in float fUseOverlay;
in float fAo;
flat in uint fTileBase;
flat in uint fTileOverlay;
flat in uint fNormalIndex;

out vec4 FragColor;

// FIXED: Defined array properly with vec3 constructors and moved above functions
// +Y -Y +X -X +Z -Z
const vec3 NORMALS[6] = vec3[6](
    vec3( 0.0f,  1.0f,  0.0f), vec3( 0.0f, -1.0f,  0.0f),   // +Y -Y 
    vec3( 1.0f,  0.0f,  0.0f), vec3(-1.0f,  0.0f,  0.0f),   // +X -X 
    vec3( 0.0f,  0.0f,  1.0f), vec3( 0.0f,  0.0f, -1.0f)    // +Z -Z 
);

// --- Tile-local UV -> atlas UV -------------------------------------
vec2 tileUV(vec2 tileLocal, vec2 tileMin, vec2 tileMax)
{
    return tileMin + fract(tileLocal) * (tileMax - tileMin);
}

ivec3 fragBlockPos()
{
    return ivec3(floor(fWorldPos - NORMALS[fNormalIndex] * 0.01f));
}

UVRect Tile(uint i)
{
    const float TILE_W = 0.0625f;
    const float TILE_H = 0.0625f;

    uint col = i % 16u;
    uint row = i / 16u;

    float u0 = float(col) * TILE_W;
    float v0 = 1.0f - (float(row) + 1.0f) * TILE_H;

    UVRect uv;
    uv._min = vec2(u0, v0);
    uv._max = vec2(u0 + TILE_W, v0 + TILE_H);

    return uv;
}

void main()
{
    // Construct baseTileMin, baseTileMax, overlayTileMin, overlayTileMax
    UVRect baseUV = Tile(fTileBase);
    UVRect overlayUV = Tile(fTileOverlay);

    vec2 baseTileMin = baseUV._min;
    vec2 baseTileMax = baseUV._max;
    vec2 overlayTileMin = overlayUV._min;
    vec2 overlayTileMax = overlayUV._max;

    // --- Base texture -------------------------------------
    vec2 atlasUV = tileUV(fUV, baseTileMin, baseTileMax);

    // Correct mipmap derivatives — from smooth tile-local
    vec2 tileSize = baseTileMax - baseTileMin;
    vec2 dx = dFdx(fUV) * tileSize;
    vec2 dy = dFdy(fUV) * tileSize;

    vec4 baseTex = textureGrad(u_atlas, atlasUV, dx, dy);

    if (baseTex.a < 0.1)
        discard;

    vec3 color = baseTex.rgb;
    
    // Overlay
    if (fUseOverlay > 0.5f) {
        vec2 overlayAtlasUV = tileUV(fUV, overlayTileMin, overlayTileMax);
        vec2 overlaySize = overlayTileMax - overlayTileMin;
        vec2 overlayDx = dFdx(fUV) * overlaySize;
        vec2 overlayDy = dFdy(fUV) * overlaySize;
        vec4 overlayTex = textureGrad(u_atlas, overlayAtlasUV, overlayDx, overlayDy);
        color = mix(color, overlayTex.rgb * fTint, overlayTex.a);
    } else {
        color *= fTint;
    }

    // --- Diffuse lighting (flat per-face) -------------------------------------
    float NdotL = max(dot(normalize(NORMALS[fNormalIndex]), -normalize(u_lightDir)), 0.0f);
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