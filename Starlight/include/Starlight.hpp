#pragma once

/*#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d12.h>*/

struct ID3D12Device;
struct ID3D12Device5;
struct ID3D12CommandQueue;
struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList4;
struct ID3D12Fence;
struct ID3D12DescriptorHeap;
struct ID3D12Resource;
struct ID3D12RootSignature;
struct ID3D12PipelineState;
struct D3D12_RAYTRACING_INSTANCE_DESC;
typedef void* HANDLE;

struct LoadedMesh;

#include "DirectXMath.h"

#define STARLIGHT_MAX_MESHES (0xFF)
#define STARLIGHT_BUFFER_COUNT (2)

struct MeshFormat
{
	size_t VertexSize;
	size_t IndexSize;

	size_t PositionOffset; // Number of bytes from the beginning of a vertex to the beginning of the position vector
	size_t NormalOffset; // Number of bytes from the beginning of a vertex to the beginning of the normal vector
	size_t TangentOffset; // Number of bytes from the beginning of a vertex to the beginning of the tangent vector
	size_t TexCoordOffset; // Number of bytes from the beginning of a vertex to the beginning of the texture coordinate
	size_t LightmapCoordOffset;
};

struct MeshInfo
{
	const void* VertexData;
	size_t VertexSize;
	size_t VertexCount;

	const void* IndexData;
	size_t IndexSize;
	size_t IndexCount;

	DirectX::XMFLOAT4X4 Transform;
};

// Describes a single-mip R8G8B8A8 input texture.
struct TextureInfo
{
	const void* PixelData;
	size_t Stride; // The number of bytes from the beginning of one row of pixel data to the beginning of the next row of pixel data.
	size_t Width;
	size_t Height;
};

struct MaterialInfo
{
	TextureInfo DiffuseTexture;
	TextureInfo NormalRoughnessMetallicTexture;
	DirectX::XMFLOAT3 DiffuseTint;
	// TODO: Add emissive material support
};

struct PointLight
{
	DirectX::XMFLOAT3 Position;
	float Radius;
	float AttenuationConstant;
	float AttenuationLinear;
	float AttenuationQuadratic;
};

struct DirectionalLight
{
	DirectX::XMFLOAT3 Direction;
};

struct SkyLight
{
	const void* FacePixelData[6];
	size_t Stride;
	size_t Resolution;
};

struct OutputBuffer
{
	void* PixelData;
	size_t Stride; // The number of bytes from the beginning of one row of pixel data to the beginning of the next row of pixel data.
	size_t Width;
	size_t Height;
};

class Starlight
{
private:
	ID3D12Device5* _Device;
	ID3D12CommandQueue* _CommandQueue = nullptr;
	ID3D12CommandAllocator* _CommandAllocator = nullptr;
	ID3D12GraphicsCommandList4* _CommandList = nullptr;
	ID3D12Fence* _Fence = nullptr;
	HANDLE _FenceEvent;
	uint64_t _FenceValue = 1;

	ID3D12DescriptorHeap* _RTVDescriptorHeap = nullptr;
	ID3D12DescriptorHeap* _ResourceDescriptorHeap = nullptr;
	ID3D12DescriptorHeap* _SamplerDescriptorHeap = nullptr;
	unsigned int _RTVDescriptorSize = 0;
	unsigned int _SRVDescriptorSize = 0;

	ID3D12Resource* _LightmaskBuffers[STARLIGHT_BUFFER_COUNT] = { };
	size_t _LatestLightmaskBuffer = 0;
	ID3D12Resource* _ReadbackBuffer = nullptr;

	// Raytracing resources
	ID3D12Resource* _SceneAccelerationStructure = nullptr; // The top-level acceleration structure for DXR
	ID3D12Resource* _SceneBuildScratchBuffer = nullptr; // Scratch buffer used when building `_SceneAccelerationStructure`
	ID3D12Resource* _SceneInstanceBuffer = nullptr; // The buffer that contains the instances of the meshes, for building `_SceneAccelerationStructure`
	D3D12_RAYTRACING_INSTANCE_DESC* _SceneInstanceData = nullptr; // The mapped contents of `_SceneInstanceBuffer`
	bool _RebuildTopLevel = true;

	// Point light baking
	ID3D12Resource* _PointLightConstantBuffer = nullptr;
	struct PointLightConstants* _PointLightConstantData = nullptr;
	ID3D12RootSignature* _PointLightRootSignature = nullptr;
	ID3D12PipelineState* _PointLightPipelineState = nullptr;

	// Directional light baking
	ID3D12Resource* _DirectionalLightConstantBuffer = nullptr;
	struct DirectionalLightConstants* _DirectionalLightConstantData = nullptr;
	ID3D12RootSignature* _DirectionalLightRootSignature = nullptr;
	ID3D12PipelineState* _DirectionalLightPipelineState = nullptr;

	MeshFormat _MeshFormat;
	LoadedMesh* _Meshes;
	size_t _MeshCount;

	Starlight(ID3D12Device* device, const MeshFormat& meshFormat);

	/**
	 * Prepares Starlight to bake into the given output buffer.
	 */
	void PrepareOutputBuffer(const OutputBuffer* outputBuffer);
	/**
	 * Downloads the bake into the given output buffer and disposes the internal resource.
	 */
	void CleanupOutputBuffer(const OutputBuffer* outputBuffer);
	void ExecuteAndReset();
	void TransitionResource(ID3D12Resource* resource, enum D3D12_RESOURCE_STATES before, enum D3D12_RESOURCE_STATES after);
	void RebuildTopLevelIfNeeded();
	void BakeLight(struct ID3D12PipelineState* pipelineState, struct ID3D12RootSignature* rootSignature, enum class BakeResource constantBuffer, const OutputBuffer* outputBuffer);
	ID3D12Resource* CreateBuffer(size_t size, const char* data);
	ID3D12Resource* CreateTexture(size_t width, size_t height, size_t stride, enum DXGI_FORMAT format, const char* data);
	ID3D12Resource* CreateResource(const struct D3D12_RESOURCE_DESC* description, size_t stride, size_t dataSize, const char* data);
	ID3D12Resource* CreateUAV(size_t size, enum D3D12_RESOURCE_STATES initialState);
	ID3D12Resource* CreateConstantBuffer(size_t size);
	ID3D12RootSignature* CreateRootSignature(const struct D3D12_ROOT_SIGNATURE_DESC* desc);
	struct D3D12_CPU_DESCRIPTOR_HANDLE GetBakeResourceCPU(enum class BakeResource resource) const;
	struct D3D12_GPU_DESCRIPTOR_HANDLE GetBakeResourceGPU(enum class BakeResource resource) const;
	struct D3D12_CPU_DESCRIPTOR_HANDLE GetMeshResourceCPU(size_t mesh, enum class MeshResource resource) const;
	struct D3D12_GPU_DESCRIPTOR_HANDLE GetMeshResourceGPU(size_t mesh, enum class MeshResource resource) const;

public:
	~Starlight();

	inline const MeshFormat& GetMeshFormat() const { return this->_MeshFormat; }

	void AddMesh(const MeshInfo* meshInfo, const MaterialInfo* materialInfo);
	void BakePointLight(const PointLight* pointLight, const OutputBuffer* outputBuffer);
	void BakeDirectionalLight(const DirectionalLight* directionalLight, const OutputBuffer* outputBuffer);
	void BakeSkyLight(const SkyLight* skyLight, const OutputBuffer* outputBuffer);

	static Starlight* CreateStarlight(ID3D12Device* device, const MeshFormat& meshFormat);
};