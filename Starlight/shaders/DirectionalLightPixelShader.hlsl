#include "PBR.hlsl"

#define DIRECTIONAL_SHADOW_SAMPLE_COUNT (1000)

cbuffer PointLightConstantBuffer : register(b0)
{
    float3 WorldDirection;
    float _Padding;
}

float3 DirectionalLightIrradiance(float3 position, float3 normal, float3 tangent, float3 bitangent, float3 lightDirection, float2 coord)
{
    float3 lightForward = normalize(lightDirection);
    float3 lightSide = cross(lightForward, float3(0, 1, 0));
    float3 lightUp = cross(lightSide, lightForward);
    
    float softness = 0.1f;
    
    float visibility = 0.0f;
    for (uint i = 0; i < DIRECTIONAL_SHADOW_SAMPLE_COUNT; i++)
    {
        float2 hammersley = Hammersley2dRandomized(i, DIRECTIONAL_SHADOW_SAMPLE_COUNT, coord);
        float3 localDirection = HemisphereProjectionCosine(hammersley.x, hammersley.y);
        
        float3 blurredDirection = localDirection.x * lightSide + localDirection.y * lightUp + localDirection.z * lightForward;
        blurredDirection = normalize(lerp(lightForward, blurredDirection, softness));
        float3 jitteredPosition = JitterPosition(position, tangent, bitangent, i, DIRECTIONAL_SHADOW_SAMPLE_COUNT);
        float3 startPosition = jitteredPosition + normal * 0.001f;
        float3 endPoint = startPosition - normalize(blurredDirection) * 1000.0f;
        
        visibility += SampleOcclusion(startPosition, endPoint) * saturate(dot(normal, -blurredDirection));
    }
    visibility = visibility / DIRECTIONAL_SHADOW_SAMPLE_COUNT;
    
    float3 lightColor = float3(1.0f, 1.0f, 1.0f);
    float3 radiance = lightColor * visibility;
    
    // The total irradiance recieved from a point light is just the radiance - there is only the one ray contributing, all else is zero
    return radiance * 1.0f;
}

float4 PSMain(LightmapPixel input) : SV_Target0
{
    float3 directIrradiance = DirectionalLightIrradiance(input.WorldPosition, input.WorldNormal, input.WorldTangent, input.WorldBitangent, WorldDirection, input.LightmapCoord);
    
    float3 indirectIrradiance = IndirectIrradiance(input.WorldPosition, input.WorldNormal, input.WorldTangent, input.WorldBitangent, input.LightmapCoord);
    
    return float4(directIrradiance + indirectIrradiance * float3(1.0f, 1.0f, 1.0f), 1.0f);
}