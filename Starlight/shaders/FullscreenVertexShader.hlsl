#include "Common.hlsl"

ScreenPixel VSMain(uint vertexID : SV_VertexID)
{
    ScreenPixel result = (ScreenPixel)0;

    result.LightmapCoord = float2(vertexID % 2, vertexID / 2) * 2.0f;
    result.ScreenPosition = float4(result.LightmapCoord * 2.0f - float2(1.0f, 1.0f), 0.0f, 1.0f);
    result.ScreenPosition.y *= -1.0f;
    
    return result;
}