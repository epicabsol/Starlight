
#define PI (3.141592f)

struct MeshVertex
{
    float3 Position : POSITION0;
    float3 Normal : NORMAL0;
    float3 Tangent : TANGENT0;
    float2 TexCoord : TEXCOORD0;
    float2 LightmapCoord : TEXCOORD1;
};

struct ScreenPixel
{
    float4 ScreenPosition : SV_POSITION;
    float2 LightmapCoord : TEXCOORD0;
};

struct LightmapPixel
{
    float4 ScreenPosition : SV_POSITION;
    float3 WorldPosition : TEXCOORD2;
    float3 WorldNormal : NORMAL0;
    float3 WorldTangent : TANGENT0;
    float3 WorldBitangent : TANGENT1;
    float2 TexCoord : TEXCOORD0;
    float2 LightmapCoord : TEXCOORD1;
};

// Hammersley sequence (implementation from http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html#eq-hammersley)
float radicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

float2 hammersley2d(uint i, uint N)
{
    return float2(float(i) / float(N), radicalInverse_VdC(i));
}

// Project uniformly-distributed points onto a uniformly- or cosine-distributed hemisphere (implementation also from http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html#eq-hammersley)
float3 hemisphereProjectionUniform(float u, float v)
{
    float phi = v * 2.0 * PI;
    float cosTheta = 1.0 - u;
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    return float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

float3 hemisphereProjectionCosine(float u, float v)
{
    float phi = v * 2.0 * PI;
    float cosTheta = sqrt(1.0 - u);
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    return float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

float3 JitterPosition(float3 position, float3 tangent, float3 bitangent, uint sample, uint sampleCount)
{
    // Approximate the world-space width and height of one lightmap texel
    float texelSize = lerp(length(ddx_fine(position)), length(ddy_fine(position)), 0.5f);
    
    float2 jitter = hammersley2d(sample, sampleCount) - float2(0.5f, 0.5f);
    return position + (jitter.x * tangent + jitter.y * bitangent) * texelSize;
}