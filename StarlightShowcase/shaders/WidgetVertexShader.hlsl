struct WidgetVertex
{
    float3 Position : POSITION0;
    float3 Color : COLOR0;
};

struct WidgetPixel
{
    float4 Position : SV_Position;
    float4 Color : COLOR0;
};

cbuffer FrameConstants : register(b0)
{
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
}

cbuffer WidgetConstants : register(b1)
{
    float4x4 TransformMatrix;
    float4 ColorOverride;
}

WidgetPixel VSMain(WidgetVertex vertex)
{
    WidgetPixel result = (WidgetPixel)0;

    result.Position = mul(mul(mul(float4(vertex.Position, 1.0f), TransformMatrix), ViewMatrix), ProjectionMatrix);
    //result.Color = float4(vertex.Color * Tint.xyz, 1.0f);
    result.Color = float4(lerp(vertex.Color.xyz, ColorOverride.xyz, ColorOverride.a), 1.0f);
    
    return result;
}