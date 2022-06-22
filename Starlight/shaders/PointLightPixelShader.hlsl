#include "Common.hlsl"

float4 PSMain(LightmapPixel input) : SV_Target0
{
    // TODO: Implement!
    //return float4(saturate(input.WorldPosition * 0.1f), 1.0f);
    return float4(0.0f, 0.5f, 1.0f, 1.0f);
}