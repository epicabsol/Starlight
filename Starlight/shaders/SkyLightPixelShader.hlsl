#include "PBR.hlsl"

#define SKY_SAMPLE_COUNT (1000)

TextureCube<float4> Skybox : register(t2);

float3 SkyLightIrradiance(float3 position, float3 normal, float3 tangent, float3 bitangent)
{
    float softness = 0.1f;
    
    float3 irradiance = float3(0.0f, 0.0f, 0.0f);
    for (uint i = 0; i < SKY_SAMPLE_COUNT; i++)
    {
        float2 hammersley = hammersley2d(i, SKY_SAMPLE_COUNT);
        float3 localDirection = hemisphereProjectionCosine(hammersley.x, hammersley.y);
        float3 worldDirection = localDirection.x * tangent + localDirection.y * bitangent + localDirection.z * normal;
        
        float3 radiance;
        bool hitMesh = Radiance(position + normal * 0.001f, worldDirection * 1000.0f, radiance);
        float3 skyRadiance = Skybox.Sample(LightmapSampler, worldDirection).xyz;
        
        irradiance += (hitMesh ? radiance : skyRadiance) /* * saturate(dot(normal, worldDirection))*/; // The dot product here is removed in favor of the cosine distribution above.
    }
    irradiance = irradiance / SKY_SAMPLE_COUNT;
    
    return irradiance;
}

float4 PSMain(LightmapPixel input) : SV_Target0
{
    float3 irradiance = SkyLightIrradiance(input.WorldPosition, input.WorldNormal, input.WorldTangent, input.WorldBitangent);
    
    return float4(irradiance, 1.0f);
}