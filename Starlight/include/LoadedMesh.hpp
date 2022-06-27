#pragma once

#include "DirectXMath.h"

#include <stddef.h>

struct ID3D12Resource;
enum DXGI_FORMAT;

struct MeshConstants {
	DirectX::XMFLOAT4X4 Transform;

	DirectX::XMFLOAT3 DiffuseTint;
	float _Padding;
};

struct LoadedMesh {
	ID3D12Resource* VertexBuffer;
	size_t VertexSize;
	size_t VertexCount;

	ID3D12Resource* IndexBuffer;
	DXGI_FORMAT IndexFormat;
	size_t IndexCount;

	ID3D12Resource* ConstantBuffer;

	ID3D12Resource* DiffuseTexture;
	ID3D12Resource* NormalRoughnessMetallicTexture;

	ID3D12Resource* AccelerationStructure;
};