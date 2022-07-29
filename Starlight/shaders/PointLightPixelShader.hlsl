#include "PBR.hlsl"

#define POINT_SHADOW_SAMPLE_COUNT (1000)

cbuffer PointLightConstantBuffer : register(b0)
{
    float3 WorldPosition;
    float Radius;
    float AttenuationConstant;
    float AttenuationLinear;
    float AttenuationQuadratic;
    float _Padding;
}

float3 PointLightIrradiance(float3 position, float3 normal, float3 tangent, float3 bitangent, float3 lightPosition)
{
    float3 lightForward = normalize(position - lightPosition);
    float3 lightSide = cross(lightForward, float3(0, 1, 0));
    float3 lightUp = cross(lightSide, lightForward);
    
    float visibility = 0.0f;
    for (uint i = 0; i < POINT_SHADOW_SAMPLE_COUNT; i++)
    {
        float2 hammersley = hammersley2d(i, POINT_SHADOW_SAMPLE_COUNT);
        float3 localDirection = hemisphereProjectionCosine(hammersley.x, hammersley.y);
            
        float3 lightOffset = localDirection.x * lightSide + localDirection.y * lightUp + localDirection.z * lightForward;
        float3 jitteredPosition = JitterPosition(position, tangent, bitangent, i, POINT_SHADOW_SAMPLE_COUNT);
        float3 endPoint = lightPosition + lightOffset * Radius;
        float3 startPosition = jitteredPosition + normal * 0.001f;
        
        visibility += SampleOcclusion(startPosition, endPoint);
    }
    visibility = visibility / POINT_SHADOW_SAMPLE_COUNT;
    
    float distance = length(position - lightPosition);
    float attenuation = 1.0f / (AttenuationConstant + AttenuationLinear * distance + AttenuationQuadratic * distance * distance);
    float3 lightColor = float3(1.0f, 1.0f, 1.0f);
    float3 radiance = lightColor * visibility;
    
    // The total irradiance recieved from a point light is just the radiance - there is only the one ray contributing, all else is zero
    return radiance * attenuation * saturate(dot(normal, -lightForward)) * 1.0f;
}

float4 PSMain(LightmapPixel input) : SV_Target0
{
    float3 directIrradiance = PointLightIrradiance(input.WorldPosition, input.WorldNormal, input.WorldTangent, input.WorldBitangent, WorldPosition);
    
    float3 indirectIrradiance = IndirectIrradiance(input.WorldPosition, input.WorldNormal, input.WorldTangent, input.WorldBitangent, float3(input.LightmapCoord, 0.0f));
    
    return float4(directIrradiance + indirectIrradiance, 1.0f);
}