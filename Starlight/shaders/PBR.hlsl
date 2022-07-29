
// PBR code adapted from https://learnopengl.com

#include "Common.hlsl"

#define INDIRECT_SAMPLE_COUNT (100)

float DistributionGGX(float3 N, float3 H, float a)
{
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
	
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float k)
{
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return nom / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float k)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = GeometrySchlickGGX(NdotV, k);
    float ggx2 = GeometrySchlickGGX(NdotL, k);
	
    return ggx1 * ggx2;
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    return F0 + (max(float3(1.0f - roughness, 1.0f - roughness, 1.0f - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float3 PBRDiffuse(float3 albedo, float metallic, float roughness, float3 irradiance, float3 normal, float3 view)
{
    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    F0 = lerp(F0, albedo, metallic);
    
    float3 kS = FresnelSchlickRoughness(saturate(dot(normal, view)), F0, roughness);
    float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;
    return kD * albedo / PI * irradiance; // TODO: Multiply ambient occlusion here, if desired.
}

#define VertexBufferSpace space1
#define IndexBufferSpace space2

RaytracingAccelerationStructure TLAS : register(t0);
Texture2D<float4> PreviousIrradianceTexture : register(t1);
StructuredBuffer<MeshVertex> MeshVertexBuffers[] : register(t0, VertexBufferSpace);
StructuredBuffer<uint> MeshIndexBuffers[] : register(t0, IndexBufferSpace);

SamplerState LightmapSampler : register(s0);

float4 BarycentricInterpolate4(float4 value0, float4 value1, float4 value2, float2 weights)
{
    return value0 + weights.x * (value1 - value0) + weights.y * (value2 - value0);
}

float3 BarycentricInterpolate3(float3 value0, float3 value1, float3 value2, float2 weights)
{
    return value0 + weights.x * (value1 - value0) + weights.y * (value2 - value0);
}

float2 BarycentricInterpolate2(float2 value0, float2 value1, float2 value2, float2 weights)
{
    return value0 + weights.x * (value1 - value0) + weights.y * (value2 - value0);
}

float BarycentricInterpolate(float value0, float value1, float value2, float2 weights)
{
    return value0 + weights.x * (value1 - value0) + weights.y * (value2 - value0);
}

/**
 * Calculates the radiance (total light arriving along a direction) from the given ray.
 */
float3 Radiance(float3 rayStart, float3 rayDirection, float3 ref)
{
    RayQuery<RAY_FLAG_NONE> q;
    RayDesc ray;
    ray.Origin = rayStart;
    ray.Direction = rayDirection;
    ray.TMin = 0.0f;
    ray.TMax = 1.0f;
    q.TraceRayInline(TLAS, RAY_FLAG_NONE, 0xFF, ray);
    
    q.Proceed();
    if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        uint mesh = q.CommittedInstanceIndex();
        uint tri = q.CommittedPrimitiveIndex();
        float3 position = lerp(rayStart, rayDirection, q.CommittedRayT());
        float2 barycentrics = q.CommittedTriangleBarycentrics();
        float3x4 objectToWorld = q.CommittedObjectToWorld3x4();
        
        uint index0 = MeshIndexBuffers[mesh][tri * 3];
        uint index1 = MeshIndexBuffers[mesh][tri * 3 + 1];
        uint index2 = MeshIndexBuffers[mesh][tri * 3 + 2];
        
        MeshVertex vertex0 = MeshVertexBuffers[mesh][index0];
        MeshVertex vertex1 = MeshVertexBuffers[mesh][index1];
        MeshVertex vertex2 = MeshVertexBuffers[mesh][index2];
        
        float3 normal = BarycentricInterpolate3(vertex0.Normal, vertex1.Normal, vertex2.Normal, barycentrics);
        float3 worldNormal = normalize(mul(objectToWorld, float4(normal, 0.0f)));
        float2 lightmapTexCoord = BarycentricInterpolate2(vertex0.LightmapCoord, vertex1.LightmapCoord, vertex2.LightmapCoord, barycentrics);
        float3 interpPosition = BarycentricInterpolate3(vertex0.Position, vertex1.Position, vertex2.Position, barycentrics);
        
        float3 irradiance = PreviousIrradianceTexture.Sample(LightmapSampler, lightmapTexCoord * float2(1.0f, -1.0f)).xyz;
        
        float roughness = 1.0f;
        float metallic = 0.0f;
        float3 albedo = float3(1.0f, 1.0f, 1.0f);
        
        return PBRDiffuse(albedo, metallic, roughness, irradiance, normal, normalize(-rayDirection));
    }
    else
    {
        return float3(0.0f, 0.0f, 0.0f);
    }
}

/**
 * Calculates the irradiance (total light arriving at a point) from indirect lighting (i.e. the lightmap).
 */
float3 IndirectIrradiance(float3 position, float3 normal, float3 tangent, float3 bitangent, float3 ref)
{
    float3 irradiance = float3(0.0f, 0.0f, 0.0f);

    for (uint i = 0; i < INDIRECT_SAMPLE_COUNT; i++)
    {
        float2 hammersley = hammersley2d(i, INDIRECT_SAMPLE_COUNT);
        float3 localDirection = hemisphereProjectionUniform(hammersley.x, hammersley.y);
        float3 worldDirection = localDirection.x * tangent + localDirection.y * bitangent + localDirection.z * normal;
        
        float3 radiance = Radiance(position + normal * 0.001f, worldDirection * 1000.0f, ref);
        irradiance += radiance * saturate(dot(normal, worldDirection));
    }
    irradiance *= 1.0f / INDIRECT_SAMPLE_COUNT;
    
    return irradiance;
}
