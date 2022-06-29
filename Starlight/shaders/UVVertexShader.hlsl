#include "Common.hlsl"

LightmapPixel VSMain(MeshVertex input)
{
    LightmapPixel result = (LightmapPixel)0;
	result.ScreenPosition = float4(input.LightmapCoord.x * 2.0f - 1.0f, input.LightmapCoord.y * 2.0f - 1.0f, 0.0f, 1.0f);

	// TODO: Transform object-space values into world-space
    result.WorldPosition = input.Position;
    result.WorldNormal = input.Normal;
    result.WorldTangent = input.Tangent;
    result.WorldBitangent = cross(result.WorldNormal, result.WorldTangent);
	
	result.TexCoord = input.TexCoord;
	result.LightmapCoord = input.LightmapCoord;
	return result;
}