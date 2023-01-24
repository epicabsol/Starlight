#pragma once

#include "Tool.h"

#include "DirectXMath.h"

class Mesh;

class RotateTool : public Tool
{
private:
	Mesh* _CircleMesh;

	void XM_CALLCONV DrawCircle(const DirectX::XMVECTOR position, const DirectX::XMVECTOR direction, const DirectX::XMFLOAT4& tint) const;

public:
	RotateTool(Showcase* showcase);
	virtual ~RotateTool();

	virtual const char* GetTooltip() const override;

	virtual void DrawStatus() override;
	virtual void DrawWidgets() override;

	virtual void OnMouseDown(int button, int x, int y) override;
	virtual void OnMouseUp(int button, int x, int y) override;
	virtual void OnMouseMove(int x, int y) override;
};