#pragma once

#include "Tool.h"

#include "DirectXMath.h"

class Mesh;

class RotateTool : public Tool
{
public:
	enum class RotateToolWidget
	{
		None,
		DiscX,
		DiscY,
		DiscZ,
	};

private:
	Mesh* _CircleMesh;
	Mesh* _DiscMesh;

	RotateToolWidget _ActiveWidget = RotateToolWidget::None;
	bool _IsDraggingWidget = false;
	DirectX::XMFLOAT4 _DragInitialRotation = { }; // The initial object rotation
	DirectX::XMFLOAT3 _DragInitialCursor = { }; // THe initial cursor position

	void XM_CALLCONV DrawDisc(const DirectX::XMVECTOR position, const DirectX::XMVECTOR direction, const DirectX::XMVECTOR tint, float scale) const;
	bool XM_CALLCONV HitTestDisc(const DirectX::XMVECTOR position, const DirectX::XMVECTOR direction, const DirectX::XMVECTOR rayOrigin, const DirectX::XMVECTOR rayDirection, float& t) const;
	RotateToolWidget HitTestWidgets(int x, int y, DirectX::XMVECTOR& hitPoint) const;

	DirectX::XMVECTOR XM_CALLCONV SnapToWidget(int x, int y, RotateToolWidget widget) const;

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