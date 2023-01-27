#pragma once

#include "Tool.h"

#include "DirectXMath.h"

class Mesh;

class MoveTool : public Tool
{
public:
	enum class MoveToolWidget
	{
		None,
		AxisX,
		AxisY,
		AxisZ,
		PlaneXY,
		PlayeYZ,
		PlaneXZ,
	};

private:
	Mesh* _ArrowheadMesh;
	Mesh* _ArrowLineMesh;
	MoveToolWidget _ActiveWidget = MoveToolWidget::None;

	void XM_CALLCONV DrawAxis(const DirectX::XMVECTOR position, const DirectX::XMVECTOR direction, const DirectX::XMFLOAT4& tint) const;
	bool XM_CALLCONV HitTestAxis(const DirectX::XMVECTOR position, const DirectX::XMVECTOR direction, const DirectX::XMVECTOR rayOrigin, const DirectX::XMVECTOR rayDirection, float& t) const;
	MoveToolWidget HitTestWidgets(int x, int y, DirectX::XMVECTOR& hitPoint) const;

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