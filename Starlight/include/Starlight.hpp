#pragma once

/*#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d12.h>*/

struct ID3D12Device;
struct ID3D12CommandQueue;
struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList;
struct ID3D12Fence;
struct ID3D12DescriptorHeap;
struct ID3D12Resource;
struct ID3D12RootSignature;
struct ID3D12PipelineState;
struct D3D12_RESOURCE_DESC;
typedef void* HANDLE;
enum D3D12_RESOURCE_STATES;
enum DXGI_FORMAT;

struct LoadedMesh;

#include "DirectXMath.h"

#define STARLIGHT_MAX_MESHES (0xFF)

struct MeshFormat
{
	size_t VertexSize;
	size_t IndexSize;

	size_t PositionOffset; // Number of bytes from the beginning of a vertex to the beginning of the position vector
	size_t NormalOffset; // Number of bytes from the beginning of a vertex to the beginning of the normal vector
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
	ID3D12Device* _Device;
	ID3D12CommandQueue* _CommandQueue = nullptr;
	ID3D12CommandAllocator* _CommandAllocator = nullptr;
	ID3D12GraphicsCommandList* _CommandList = nullptr;
	ID3D12Fence* _Fence = nullptr;
	HANDLE _FenceEvent;
	uint64_t _FenceValue = 1;

	ID3D12DescriptorHeap* _RTVDescriptorHeap = nullptr;
	ID3D12DescriptorHeap* _MeshDescriptorHeap = nullptr;
	unsigned int _RTVDescriptorSize = 0;
	unsigned int _SRVDescriptorSize = 0;

	ID3D12Resource* _LightmaskBuffer = nullptr;
	ID3D12Resource* _ReadbackBuffer = nullptr;

	ID3D12RootSignature* _RootSignature = nullptr;
	ID3D12PipelineState* _PointLightPipelineState = nullptr;

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
	void TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
	ID3D12Resource* CreateBuffer(size_t size, const char* data);
	ID3D12Resource* CreateTexture(size_t width, size_t height, size_t stride, DXGI_FORMAT format, const char* data);
	ID3D12Resource* CreateResource(const D3D12_RESOURCE_DESC* description, size_t stride, size_t dataSize, const char* data);

public:
	~Starlight();

	inline const MeshFormat& GetMeshFormat() const { return this->_MeshFormat; }

	void AddMesh(const MeshInfo* meshInfo, const MaterialInfo* materialInfo);
	void BakePointLight(const PointLight* pointLight, const OutputBuffer* outputBuffer);
	void BakeDirectionalLight(const DirectionalLight* directionalLight, const OutputBuffer* outputBuffer);
	void BakeSkyLight(const SkyLight* skyLight, const OutputBuffer* outputBuffer);

	static Starlight* CreateStarlight(ID3D12Device* device, const MeshFormat& meshFormat);
};