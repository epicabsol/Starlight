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

/**
 * Creates a widget mesh in the shape of an arrowhead pointing in the +Z direction.
 */
Mesh* CreateArrowheadWidgetMesh(ID3D11Device* device, const DirectX::XMFLOAT3& color);

/**
 * Creates a widget mesh in the shape of a line from the given start point to the given end point.
 */
Mesh* CreateLineWidgetMesh(ID3D11Device* device, const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, const DirectX::XMFLOAT3& color);

/**
 * Creates a widget mesh in the shape of a circle around the Z axis.
 */
Mesh* CreateCircleWidgetMesh(ID3D11Device* device, float radius, const DirectX::XMFLOAT3& color);