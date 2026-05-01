#ifdef VERTEX_SHADER

layout(location = 0) in vec3  aPos;
layout(location = 1) in vec2  aBaseUV;
layout(location = 2) in vec2  aOverlayUV;
layout(location = 3) in vec3  aNormal;
layout(location = 4) in vec3  aBlockOrigin;
layout(location = 5) in vec3  aTint;
layout(location = 6) in float aUseOverlay;
layout(location = 7) in float a_ao;

layout(std140) uniform Matrices {
    mat4 projection;
    mat4 view;
};

out vec3  fTint;
out vec3  fNormal;
out vec2  fBaseUV;
out vec2  fOverlayUV;
out vec3  fWorldPos;
out vec3  fBlockOrigin;
out float fUseOverlay;
out float f_ao;

void main() {
    fTint        = aTint;
    fNormal      = aNormal;
    fBaseUV      = aBaseUV;
    fOverlayUV   = aOverlayUV;
    fWorldPos    = aPos;
    fUseOverlay  = aUseOverlay;
    fBlockOrigin = aBlockOrigin;
    f_ao = a_ao;
    gl_Position  = projection * view * vec4(aPos, 1.0f);
}

#endif

#ifdef FRAGMENT_SHADER

in vec2  fBaseUV;
in vec2  fOverlayUV;
in vec3  fNormal;
in vec3  fWorldPos;
in vec3  fBlockOrigin;
in vec3  fTint;
in float fUseOverlay;
in float f_ao;

uniform sampler2D u_atlas;   // unit 2

// Directional light
uniform vec3 u_lightDir;
uniform vec3 u_ambient;
uniform vec3 u_diffuse;

uniform bool  u_isSelected;
uniform ivec3 u_selectedBlock;
uniform bool u_isAOEnabled;

out vec4 FragColor;

float FaceBrightness(vec3 normal) {
    float sides = 1.0f;
    if (normal.y >  0.5f) return 1.3f;   // top    — full sky
    if (normal.y < -0.5f) return 0.50f;     // bottom — never sees sky
    if (abs(normal.x) > 0.5f) return sides;     // X sides
    return sides;                              // Z sides
}

void main() {
    // Sample texture by block type
    vec4 base = texture(u_atlas, fBaseUV);

    if (base.a < 0.5)
        discard;

    if(fUseOverlay > 0.5f)
    {
        vec4 overlay = texture(u_atlas, fOverlayUV);
        vec3 tinted = overlay.rgb * fTint;
        base.rgb = mix(base.rgb, tinted, overlay.a);
    }
    else
    {
        base.rgb *= fTint;
    }
    
    // Simple directional light
    float face = FaceBrightness(fNormal);
    float diff = max(dot(normalize(fNormal), -normalize(u_lightDir)), 0.0);
    
    // Face is base, diffuse adds on top — not stacked multipliers
    vec3 lighting = u_ambient * face + u_diffuse * diff;

    // Highlight selected block
    if(u_isSelected)
        if(ivec3(fBlockOrigin) == u_selectedBlock)
            base.rgb /= 0.85f;

    FragColor = vec4(base.rgb * lighting, base.a);

    if(u_isAOEnabled)
    {

        float ao_curved = f_ao * f_ao;   // square darkens corners more naturally
        FragColor.rgb *= mix(0.7f, 1.0f, ao_curved);
    }

    //FragColor.rgb = pow(FragColor.rgb, vec3(1.0f / 2.2f));
}

#endif