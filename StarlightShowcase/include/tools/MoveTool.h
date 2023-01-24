#pragma once

#include "Tool.h"

#include "DirectXMath.h"

class Mesh;

class MoveTool : public Tool
{
private:
	Mesh* _ArrowheadMesh;
	Mesh* _ArrowLineMesh;

	void XM_CALLCONV DrawAxis(const DirectX::XMVECTOR position, const DirectX::XMVECTOR direction, const DirectX::XMFLOAT4& tint) const;

public:
	MoveTool(Showcase* showcase);
	virtual ~MoveTool();

	virtual const char* GetTooltip() const override;

	virtual void DrawStatus() override;
	virtual void DrawWidgets() override;

	virtual void OnMouseDown(int button, int x, int y) override;
	virtual void OnMouseUp(int button, int x, int y) override;
	virtual void OnMouseMove(int x, int y) override;
};