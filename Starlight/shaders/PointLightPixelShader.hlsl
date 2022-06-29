#include "Common.hlsl"

RaytracingAccelerationStructure TLAS : register(t0);

#define SAMPLE_COUNT (1000)

float SampleOcclusion(float3 start, float3 direction)
{
    RayQuery<RAY_FLAG_NONE> q;
    RayDesc ray;
    ray.Origin = start;
    ray.Direction = direction;
    ray.TMin = 0.0f;
    ray.TMax = 1000.0f;
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
    float total = 0.0f;
    for (uint i = 0; i < SAMPLE_COUNT; i++)
    {
        float2 hammersley = hammersley2d(i, SAMPLE_COUNT);
        float3 localDirection = hemisphereProjectionUniform(hammersley.x, hammersley.y);
            
        float3 worldDirection = localDirection.x * input.WorldTangent + localDirection.y * input.WorldBitangent + localDirection.z * input.WorldNormal;

        total += SampleOcclusion(input.WorldPosition + input.WorldNormal * 0.001f, worldDirection);

    }
    total = total / SAMPLE_COUNT;
    return float4(total, total, total, 1.0f);

}