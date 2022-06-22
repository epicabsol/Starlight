
struct MeshVertex
{
    float3 Position : POSITION0;
    float3 Normal : NORMAL0;
    float2 TexCoord : TEXCOORD0;
    float2 LightmapCoord : TEXCOORD1;
};

struct LightmapPixel
{
    float4 ScreenPosition : SV_POSITION;
    float3 WorldPosition : TEXCOORD2;
    float3 WorldNormal : NORMAL0;
    float2 TexCoord : TEXCOORD0;
    float2 LightmapCoord : TEXCOORD1;
};