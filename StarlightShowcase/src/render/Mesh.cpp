#include "render/Mesh.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d11.h>

Mesh::Mesh(ID3D11Device* device, void* vertexData, size_t vertexSize, size_t vertexCount, void* indexData, size_t indexCount, DXGI_FORMAT indexFormat, D3D11_PRIMITIVE_TOPOLOGY topology)
	: _VertexCount(vertexCount), _IndexCount(indexCount), _IndexFormat(indexFormat), _Topology(topology)
{
	D3D11_BUFFER_DESC bufferDesc = { (UINT)(vertexSize * vertexCount), D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, 0, (UINT)(vertexSize) };
	D3D11_SUBRESOURCE_DATA bufferData = { vertexData, 0, 0 };
	HRESULT hr = device->CreateBuffer(&bufferDesc, &bufferData, &this->_VertexBuffer);

	UINT indexSize = 0;
	switch (indexFormat)
	{
	case DXGI_FORMAT_R8_UINT:
		indexSize = 1;
		break;
	case DXGI_FORMAT_R16_UINT:
		indexSize = 2;
		break;
	case DXGI_FORMAT_R32_UINT:
		indexSize = 4;
		break;
	}
	bufferDesc = { (UINT)(indexSize * indexCount), D3D11_USAGE_IMMUTABLE, D3D11_BIND_INDEX_BUFFER, 0, 0, indexSize };
	bufferData = { indexData, 0, 0 };
	hr = device->CreateBuffer(&bufferDesc, &bufferData, &this->_IndexBuffer);
}

Mesh::~Mesh()
{
	if (this->_VertexBuffer != nullptr)
	{
		this->_VertexBuffer->Release();
		this->_VertexBuffer = nullptr;
	}
	if (this->_IndexBuffer != nullptr)
	{
		this->_IndexBuffer->Release();
		this->_IndexBuffer = nullptr;
	}
}
