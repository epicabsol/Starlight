

struct VS_IN
{
	float3 Position : POSITION0;
	float3 Normal : NORMAL0;
	float2 TexCoord : TEXCOORD0;
	float2 LightmapCoord : TEXCOORD1;
};

struct PS_IN
{
	float4 ScreenPosition : SV_POSITION;
	float3 WorldPosition : TEXCOORD2;
	float3 WorldNormal : NORMAL0;
	float2 TexCoord : TEXCOORD0;
	float2 LightmapCoord : TEXCOORD1;
};

PS_IN VSMain(VS_IN input)
{
    PS_IN result = (PS_IN)0;
	result.ScreenPosition = float4(input.LightmapCoord.x, input.LightmapCoord.y, 0.0f, 1.0f);

	result.TexCoord = input.TexCoord;
	result.LightmapCoord = input.LightmapCoord;
	return result;
}