#include "Common.hlsl"

Texture2D<float4> PreviousIrradianceTexture : register(t0);
SamplerState StandardSampler : register(s0);

float4 PSMain(ScreenPixel input) : SV_Target0
{
    float2 pixelSize = float2(ddx(input.LightmapCoord.x), ddy(input.LightmapCoord.y));
    
    float4 oldValue = PreviousIrradianceTexture.Sample(StandardSampler, input.LightmapCoord);
    
    float pixels = 0.0f;
    float3 total = float3(0.0f, 0.0f, 0.0f);
    
    for (int dx = -1; dx <= 1; dx++)
    {
        for (int dy = -1; dy <= 1; dy++)
        {
            float4 neighborValue = PreviousIrradianceTexture.Sample(StandardSampler, input.LightmapCoord + pixelSize * float2(dx, dy));
            total += neighborValue.xyz * neighborValue.w;
            pixels += neighborValue.w;
        }
    }

    return oldValue.w == 0.0f ? float4(total / pixels, 1.0f) : oldValue;
}