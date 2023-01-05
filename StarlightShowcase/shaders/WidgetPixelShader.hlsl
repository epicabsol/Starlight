struct WidgetPixel
{
    float4 Position : SV_Position;
    float4 Color : COLOR0;
};

float4 PSMain(WidgetPixel pixel) : SV_Target0
{
    return pixel.Color;
}