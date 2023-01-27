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
	bool _IsDraggingWidget = false;
	DirectX::XMFLOAT3 _DragInitialPosition = { }; // The initial object position
	DirectX::XMFLOAT3 _DragInitialCursor = { }; // The initial cursor position

	void XM_CALLCONV DrawAxis(const DirectX::XMVECTOR position, const DirectX::XMVECTOR direction, const DirectX::XMVECTOR tint) const;
	bool XM_CALLCONV HitTestAxis(const DirectX::XMVECTOR position, const DirectX::XMVECTOR direction, const DirectX::XMVECTOR rayOrigin, const DirectX::XMVECTOR rayDirection, float& t) const;
	MoveToolWidget HitTestWidgets(int x, int y, DirectX::XMVECTOR& hitPoint) const;

	/**
	 * Snaps the given mouse coordinates to a world position according to the given widget.
	 * For axis widgets, this snaps to the axis. For plane widgets, this snaps to the plane.
	 */
	DirectX::XMVECTOR XM_CALLCONV SnapToWidget(int x, int y, MoveToolWidget widget) const;

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