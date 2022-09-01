
// PBR code adapted from https://learnopengl.com

#include "Common.hlsl"

#define INDIRECT_SAMPLE_COUNT (20)
#define SPECULAR_SAMPLE_COUNT (20)

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

/*float3 ImportanceSampleGGX(float2 Xi, float3 normal, float roughness)
{
    float a = roughness * roughness; // Roughness squaring looks better according to Disney & Epic
    
    float phi = 2.0f * PI * Xi.x;
    float cosTheta = sqrt((1.0f - Xi.y) / (1.0f + (a * a - 1.0f) * Xi.y));
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

    // Spherical to Cartesian
    float3 H = float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

}*/

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
#define DiffuseTextureSpace space3
#define NRMTextureSpace space4

RaytracingAccelerationStructure TLAS : register(t0);
Texture2D<float4> PreviousIrradianceTexture : register(t1);
StructuredBuffer<MeshVertex> MeshVertexBuffers[] : register(t0, VertexBufferSpace);
StructuredBuffer<uint> MeshIndexBuffers[] : register(t0, IndexBufferSpace);
Texture2D<float4> MeshDiffuseTextures[] : register(t0, DiffuseTextureSpace);
Texture2D<float4> MeshNRMTextures[] : register(t0, NRMTextureSpace);

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
bool Radiance(float3 rayStart, float3 rayDirection, float2 coord,
    out float3 albedo, out float roughness, out float metallic, out float3 normal, out float3 tangent, out float3 irradiance)
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
        
        uint index0 = MeshIndexBuffers[NonUniformResourceIndex(mesh)][tri * 3];
        uint index1 = MeshIndexBuffers[NonUniformResourceIndex(mesh)][tri * 3 + 1];
        uint index2 = MeshIndexBuffers[NonUniformResourceIndex(mesh)][tri * 3 + 2];
        
        MeshVertex vertex0 = MeshVertexBuffers[NonUniformResourceIndex(mesh)][index0];
        MeshVertex vertex1 = MeshVertexBuffers[NonUniformResourceIndex(mesh)][index1];
        MeshVertex vertex2 = MeshVertexBuffers[NonUniformResourceIndex(mesh)][index2];
        
        normal = BarycentricInterpolate3(vertex0.Normal, vertex1.Normal, vertex2.Normal, barycentrics);
        float3 worldNormal = normalize(mul(objectToWorld, float4(normal, 0.0f)));
        float2 lightmapTexCoord = BarycentricInterpolate2(vertex0.LightmapCoord, vertex1.LightmapCoord, vertex2.LightmapCoord, barycentrics);
        float2 texCoord = BarycentricInterpolate2(vertex0.TexCoord, vertex1.TexCoord, vertex2.TexCoord, barycentrics);
        tangent = BarycentricInterpolate3(vertex0.Tangent, vertex1.Tangent, vertex2.Tangent, barycentrics);
        //float3 bitangent = cross(normal, tangent);
        
        irradiance = PreviousIrradianceTexture.Sample(LightmapSampler, lightmapTexCoord * float2(1.0f, -1.0f)).xyz;
        
        float4 diffuseTexture = MeshDiffuseTextures[NonUniformResourceIndex(mesh)].Sample(LightmapSampler, texCoord * float2(1.0f, -1.0f));
        float4 normalRoughnessMetallic = MeshNRMTextures[NonUniformResourceIndex(mesh)].Sample(LightmapSampler, texCoord * float2(1.0f, -1.0f));
        
        albedo = ToLinearSpace(diffuseTexture.xyz);
        float3 tangentNormal = float3(normalRoughnessMetallic.xy * 2.0f - 1.0f, 0.0f);
        tangentNormal.z = sqrt(1.0f - tangentNormal.x * tangentNormal.x - tangentNormal.y * tangentNormal.y);
        roughness = normalRoughnessMetallic.z;
        metallic = normalRoughnessMetallic.w;
        
        //float3 pbrDiffuse = PBRDiffuse(albedo, metallic, roughness, irradiance, normal, normalize(-rayDirection));
        
        //radiance = pbrDiffuse;
        //return true;
        
        return true;
        
        /*if (recurse)
        {
            //radiance = irradiance * albedo;
            radiance = pbrDiffuse; // TODO: Pick a better approximation?
        }
        else
        {
            float3 pbrSpecular = PBRSpecular(albedo, metallic, roughness, rayStart, normal, tangent, bitangent, normalize(-rayDirection), coord);
            radiance = pbrDiffuse + pbrSpecular;
        }
        
        return true;*/
    }
    else
    {
        albedo = 0.0f;
        roughness = 0.0f;
        metallic = 0.0f;
        normal = 0.0f;
        tangent = 0.0f;
        irradiance = 0.0f;
        
        //radiance = float3(0.0f, 0.0f, 0.0f);
        return false;
    }
}

bool RadianceNoSpec(float3 rayStart, float3 rayDirection, out float3 radiance, float2 coord)
{
    float3 albedo;
    float roughness;
    float metallic;
    float3 normal;
    float3 tangent;
    float3 irradiance;
    
    bool hit = Radiance(rayStart, rayDirection, coord, albedo, roughness, metallic, normal, tangent, irradiance);
    if (hit)
    {
        float3 pbrDiffuse = PBRDiffuse(albedo, metallic, roughness, irradiance, normal, normalize(-rayDirection));
        //float3 pbrSpecular = PBRSpecular(albedo, metallic, roughness, rayStart, normal, tangent, bitangent, normalize(-rayDirection), coord);
        //radiance = pbrDiffuse + pbrSpecular;
        //radiance = pbrDiffuse; // TODO: Pick a better approximation?
        radiance = irradiance * albedo * 100.0f;
        //radiance = 1.0f;
        return true;
    }
    else
    {
        radiance = 0.0f;
        return false;
    }
}

float3 PBRSpecular(float3 albedo, float metallic, float roughness, float3 position, float3 normal, float3 tangent, float3 bitangent, float3 view, float2 coord)
{
    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    F0 = lerp(F0, albedo, metallic);
    
    float3 kS = FresnelSchlickRoughness(saturate(dot(normal, view)), F0, roughness);
    
    
    float3 irradiance = float3(0.0f, 0.0f, 0.0f);
    
    // wo = view

    // Loop through incoming light directions
    for (uint i = 0; i < SPECULAR_SAMPLE_COUNT; i++)
    {
        float2 hammersley = Hammersley2dRandomized(i, SPECULAR_SAMPLE_COUNT, coord);
        float3 localDirection = HemisphereProjectionUniform(hammersley.x, hammersley.y);
        float3 worldDirection = localDirection.x * tangent + localDirection.y * bitangent + localDirection.z * normal; // wi
        
        float3 radiance; // Li(p, wi)
        RadianceNoSpec(position + normal * 0.001f, worldDirection * 1000.0f, radiance, coord);
        
        
        float3 H = normalize(view + worldDirection); // H = halfway between view vector and light vector
        float D = saturate(DistributionGGX(normal, H, roughness)); // Distribution function
        float3 F = saturate(FresnelSchlick(saturate(dot(H, view)), F0)); // Fresnel function
        float G = saturate(GeometrySmith(normal, view, worldDirection, roughness)); // Geometry function (Smith)
        
        irradiance += kS * D * F * G / (4.0f * dot(view, normal) * dot(worldDirection, normal)) * radiance * saturate(dot(normal, worldDirection));
    }
    irradiance *= 1.0f / SPECULAR_SAMPLE_COUNT;
    
    return irradiance;
}


bool RadianceFull(float3 rayStart, float3 rayDirection, out float3 radiance, float2 coord)
{
    float3 albedo;
    float roughness;
    float metallic;
    float3 normal;
    float3 tangent;
    float3 irradiance;
    
    bool hit = Radiance(rayStart, rayDirection, coord, albedo, roughness, metallic, normal, tangent, irradiance);
    if (hit)
    {
        float3 bitangent = cross(normal, tangent);
        float3 pbrDiffuse = PBRDiffuse(albedo, metallic, roughness, irradiance, normal, normalize(-rayDirection));
        float3 pbrSpecular = PBRSpecular(albedo, metallic, roughness, rayStart, normal, tangent, bitangent, normalize(-rayDirection), coord);
        radiance = pbrDiffuse + pbrSpecular;
        return true;
    }
    else
    {
        radiance = 0.0f;
        return false;
    }
}

bool RadianceOnlySpec(float3 rayStart, float3 rayDirection, out float3 radiance, float2 coord)
{
    float3 albedo;
    float roughness;
    float metallic;
    float3 normal;
    float3 tangent;
    float3 irradiance;
    
    bool hit = Radiance(rayStart, rayDirection, coord, albedo, roughness, metallic, normal, tangent, irradiance);
    if (hit)
    {
        float3 bitangent = cross(normal, tangent);
        float3 pbrSpecular = PBRSpecular(albedo, metallic, roughness, rayStart, normal, tangent, bitangent, normalize(-rayDirection), coord);
        radiance = pbrSpecular;
        return true;
    }
    else
    {
        radiance = 0.0f;
        return false;
    }
}

/**
 * Calculates the irradiance (total light arriving at a point) from indirect lighting (i.e. the lightmap).
 */
float3 IndirectIrradiance(float3 position, float3 normal, float3 tangent, float3 bitangent, float2 coord)
{
    float3 irradiance = float3(0.0f, 0.0f, 0.0f);

    for (uint i = 0; i < INDIRECT_SAMPLE_COUNT; i++)
    {
        float2 hammersley = Hammersley2dRandomized(i, INDIRECT_SAMPLE_COUNT, coord);
        float3 localDirection = HemisphereProjectionUniform(hammersley.x, hammersley.y);
        float3 worldDirection = localDirection.x * tangent + localDirection.y * bitangent + localDirection.z * normal;
        
        float3 radiance;
        RadianceOnlySpec(position + normal * 0.001f, worldDirection * 1000.0f, radiance, coord);
        irradiance += radiance * saturate(dot(normal, worldDirection));
    }
    irradiance *= 1.0f / INDIRECT_SAMPLE_COUNT;
    
    return irradiance;
}

/**
 * Determines whether there is any geometry occluding the segment from start to end. (0.0f: occluded, 1.0f: not occluded)
 */
float SampleOcclusion(float3 start, float3 end)
{
    RayQuery<RAY_FLAG_NONE> q;
    RayDesc ray;
    ray.Origin = start;
    ray.Direction = end - start;
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
