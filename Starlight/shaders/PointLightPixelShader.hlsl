#include "Common.hlsl"

#define SAMPLE_COUNT (1000)

RaytracingAccelerationStructure TLAS : register(t0);

cbuffer PointLightConstantBuffer : register(b0)
{
    float3 WorldPosition;
    float Radius;
    float AttenuationConstant;
    float AttenuationLinear;
    float AttenuationQuadratic;
    float _Padding;
}

float SampleOcclusion(float3 start, float3 direction)
{
    RayQuery<RAY_FLAG_NONE> q;
    RayDesc ray;
    ray.Origin = start;
    ray.Direction = direction;
    ray.TMin = 0.0f;
    ray.TMax = 1.0f;
    q.TraceRayInline(TLAS, RAY_FLAG_NONE, 0xFF, ray);
    
    q.Proceed();
    if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        return 0.0f;

    }
    else
    {
        return 1.0f;

    }
    
}

float4 PSMain(LightmapPixel input) : SV_Target0
{
    float3 lightForward = normalize(input.WorldPosition - WorldPosition);
    float3 lightSide = cross(lightForward, float3(0, 1, 0));
    float3 lightUp = cross(lightSide, lightForward);
    
    float visibility = 0.0f;
    for (uint i = 0; i < SAMPLE_COUNT; i++)
    {
        float2 hammersley = hammersley2d(i, SAMPLE_COUNT);
        float3 localDirection = hemisphereProjectionUniform(hammersley.x, hammersley.y);
            
        float3 lightOffset = localDirection.x * lightSide + localDirection.y * lightUp + localDirection.z * lightForward;
        float3 endPoint = WorldPosition + lightOffset * Radius;
        float3 startPosition = input.WorldPosition + input.WorldNormal * 0.001f;
        
        visibility += SampleOcclusion(startPosition, endPoint - startPosition);
    }
    visibility = visibility / SAMPLE_COUNT;
    
    float distance = length(input.WorldPosition - WorldPosition);
    float attenuation = 1.0f / (AttenuationConstant + AttenuationLinear * distance + AttenuationQuadratic * distance * distance);
    float3 lightColor = float3(1.0f, 1.0f, 1.0f);
    float3 radiance = lightColor * attenuation * saturate(dot(input.WorldNormal, -lightForward));     
    
    // The total irradiance recieved from a point light is just the radiance - there is only the one ray contributing, all else is zero
    float3 irradiance = radiance * visibility;
    
    return float4(irradiance, 1.0f);
}