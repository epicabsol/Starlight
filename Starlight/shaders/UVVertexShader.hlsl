#include "Common.hlsl"

LightmapPixel VSMain(MeshVertex input)
{
    LightmapPixel result = (LightmapPixel)0;
	result.ScreenPosition = float4(input.LightmapCoord.x * 2.0f - 1.0f, input.LightmapCoord.y * -2.0f + 1.0f, 0.0f, 1.0f);

	result.TexCoord = input.TexCoord;
	result.LightmapCoord = input.LightmapCoord;
	return result;
}