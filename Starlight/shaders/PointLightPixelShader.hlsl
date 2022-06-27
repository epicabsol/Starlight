#include "Common.hlsl"

RaytracingAccelerationStructure TLAS : register(t0);

float4 PSMain(LightmapPixel input) : SV_Target0
{
    RayQuery<RAY_FLAG_NONE> q;
    RayDesc ray;
    ray.Origin = input.WorldPosition + input.WorldNormal * 0.001f;
    ray.Direction = input.WorldNormal;
    ray.TMin = 0.0f;
    ray.TMax = 1000.0f;
    q.TraceRayInline(TLAS, RAY_FLAG_NONE, 0xFF, ray);
    
    q.Proceed();
    if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        return float4(0.0f, 1.0f, 0.0f, 1.0f);

    }
    else
    {
        return float4(0.0f, 0.0f, 1.0f, 1.0f);

    }
}