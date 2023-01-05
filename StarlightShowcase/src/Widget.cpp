#include "Widget.h"

#include "render/Mesh.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d11.h>

#define GRID_SIZE (10)

using namespace DirectX;

D3D11_INPUT_ELEMENT_DESC WidgetVertexElements[]
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};
size_t WidgetVertexElementCount = sizeof(WidgetVertexElements) / sizeof(WidgetVertexElements[0]);

Mesh* CreateGridWidgetMesh(ID3D11Device* device)
{
	constexpr size_t parallelVertexCount = (GRID_SIZE * 2 + 1) * 2;
	constexpr size_t vertexCount = parallelVertexCount * 2;
	constexpr XMFLOAT3 color = XMFLOAT3(0.5f, 0.5f, 0.5f);

	WidgetVertex* vertices = new WidgetVertex[vertexCount];
	uint16_t* indices = new uint16_t[vertexCount];

	for (int i = 0; i < GRID_SIZE * 2 + 1; i++)
	{
		vertices[i * 2] = { XMFLOAT3(i - GRID_SIZE, 0.0f, -GRID_SIZE), color };
		vertices[i * 2 + 1] = { XMFLOAT3(i - GRID_SIZE, 0.0f, GRID_SIZE), color };
		indices[i * 2] = i * 2;
		indices[i * 2 + 1] = i * 2 + 1;
	}

	for (int i = 0; i < GRID_SIZE * 2 + 1; i++)
	{
		vertices[parallelVertexCount + i * 2] = { XMFLOAT3(-GRID_SIZE, 0.0f, i - GRID_SIZE), color };
		vertices[parallelVertexCount + i * 2 + 1] = { XMFLOAT3(GRID_SIZE, 0.0f, i - GRID_SIZE), color };
		indices[parallelVertexCount + i * 2] = parallelVertexCount + i * 2;
		indices[parallelVertexCount + i * 2 + 1] = parallelVertexCount + i * 2 + 1;
	}

	Mesh* result = new Mesh(device, vertices, sizeof(WidgetVertex), vertexCount, indices, vertexCount, DXGI_FORMAT_R16_UINT, D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	delete[] indices;
	delete[] vertices;

	return result;
}