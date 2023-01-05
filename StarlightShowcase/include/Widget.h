#pragma once

#include <DirectXMath.h>

class Mesh;
struct ID3D11Device;
struct D3D11_INPUT_ELEMENT_DESC;

/**
 * A vertex for opaque, unlit 3D widgets
 */
struct WidgetVertex
{
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT3 Color;
};

extern D3D11_INPUT_ELEMENT_DESC WidgetVertexElements[];
extern size_t WidgetVertexElementCount;

struct WidgetConstants
{
	DirectX::XMFLOAT4X4 TransformMatrix;
	DirectX::XMFLOAT4 ColorOverride;
};

/**
 * Creates a widget mesh in the shape of a grid.
 */
Mesh* CreateGridWidgetMesh(ID3D11Device* device);
