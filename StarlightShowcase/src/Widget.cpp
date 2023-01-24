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
		XMFLOAT3 lineColor = i == GRID_SIZE ? XMFLOAT3(0.75f, 0.75f, 0.75f) : color;
		vertices[i * 2] = { XMFLOAT3(i - GRID_SIZE, 0.0f, -GRID_SIZE), lineColor };
		vertices[i * 2 + 1] = { XMFLOAT3(i - GRID_SIZE, 0.0f, GRID_SIZE), lineColor };
		indices[i * 2] = i * 2;
		indices[i * 2 + 1] = i * 2 + 1;
	}

	for (int i = 0; i < GRID_SIZE * 2 + 1; i++)
	{
		XMFLOAT3 lineColor = i == GRID_SIZE ? XMFLOAT3(0.75f, 0.75f, 0.75f) : color;
		vertices[parallelVertexCount + i * 2] = { XMFLOAT3(-GRID_SIZE, 0.0f, i - GRID_SIZE), lineColor };
		vertices[parallelVertexCount + i * 2 + 1] = { XMFLOAT3(GRID_SIZE, 0.0f, i - GRID_SIZE), lineColor };
		indices[parallelVertexCount + i * 2] = parallelVertexCount + i * 2;
		indices[parallelVertexCount + i * 2 + 1] = parallelVertexCount + i * 2 + 1;
	}

	Mesh* result = new Mesh(device, vertices, sizeof(WidgetVertex), vertexCount, indices, vertexCount, DXGI_FORMAT_R16_UINT, D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	delete[] indices;
	delete[] vertices;

	return result;
}

Mesh* CreateArrowheadWidgetMesh(ID3D11Device* device, const XMFLOAT3& color)
{
	// WARNING: This is directly proportional to the amount of stack space taken up.
	// Beware of large values incurring stack overflow.
	const int divisions = 16;
	const float thickness = 0.3f;

	WidgetVertex vertices[divisions + 2] =
	{
		{ XMFLOAT3(0.0f, 0.0f, 0.0f), color }, // Element 0 is the origin
		{ XMFLOAT3(0.0f, 0.0f, 1.0f), color }, // Element 1 is the pointed tip
	};
	uint16_t indices[divisions * 6];

	for (int i = 0; i < divisions; i++)
	{
		float angle = ((float)i / divisions) * 3.14159265f * 2.0f;
		vertices[i + 2].Position = XMFLOAT3(cos(angle) * thickness, sin(angle) * thickness, 0.0f);
		vertices[i + 2].Color = color;

		indices[i * 6 + 0] = 0;
		indices[i * 6 + 1] = 2 + i;
		indices[i * 6 + 2] = 2 + ((i + 1) % divisions);

		indices[i * 6 + 3] = 1;
		indices[i * 6 + 4] = 2 + ((i + 1) % divisions);
		indices[i * 6 + 5] = 2 + i;
	}

	Mesh* result = new Mesh(device, vertices, sizeof(WidgetVertex), divisions + 2, indices, divisions * 6, DXGI_FORMAT_R16_UINT, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	return result;
}

Mesh* CreateLineWidgetMesh(ID3D11Device* device, const XMFLOAT3& start, const XMFLOAT3& end, const XMFLOAT3& color)
{
	WidgetVertex vertices[] =
	{
		{ start, color },
		{ end, color },
	};
	uint16_t indices[] = { 0, 1 };

	return new Mesh(device, vertices, sizeof(WidgetVertex), sizeof(vertices) / sizeof(vertices[0]), indices, sizeof(indices) / sizeof(indices[0]), DXGI_FORMAT_R16_UINT, D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
}

Mesh* CreateCircleWidgetMesh(ID3D11Device* device, float radius, const DirectX::XMFLOAT3& color)
{
	const int divisions = 32;
	WidgetVertex vertices[divisions];
	uint16_t indices[divisions + 1];
	for (int i = 0; i < divisions; i++)
	{
		float angle = ((float)i / divisions) * 3.14159265f * 2.0f;
		vertices[i].Position = XMFLOAT3(cos(angle) * radius, sin(angle) * radius, 0.0f);
		vertices[i].Color = color;
		indices[i] = i;
	}
	indices[divisions] = 0;

	Mesh* result = new Mesh(device, vertices, sizeof(WidgetVertex), divisions, indices, divisions + 1, DXGI_FORMAT_R16_UINT, D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);

	return result;
}