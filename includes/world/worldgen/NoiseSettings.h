namespace WorldGenSettings
{
    constexpr float H_SCALE = 0.012f;   // was 0.006 — double freq, smaller features
    constexpr float V_SCALE = 0.010f;
    constexpr float V_BIAS = 0.012f;   // was 0.004 — stronger Y gradient, defined surface
    constexpr float SEA_LEVEL = 58.0f;

    constexpr float WARP_STRENGTH = 0.45f;   // more dramatic domain warp

    constexpr float CAVE_SCALE_A = 0.030f;   // was 0.045 — bigger tunnels
    constexpr float CAVE_SCALE_B = 0.06f;
    constexpr float CAVE_THRESHOLD = 0.10f;
    constexpr float POCKET_THRESH = 0.44f;
    constexpr int   CAVE_MIN_Y = 2;
    constexpr int   CAVE_MAX_Y = 75;       // was 60 — THIS was the main kill
}