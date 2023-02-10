#include "tools/MoveTool.h"

#include "imgui/imgui.h"

#include "render/Mesh.h"
#include "Showcase.h"
#include "Widget.h"
#include "scene/SceneObject.h"
#include "Intersection.h"

#include "DirectXCollision.h"

using namespace DirectX;

#define ARROW_STEM_LENGTH (1.0f)
#define ARROWHEAD_SIZE (0.35f)
#define ARROW_SCALE (0.1f)

struct MoveAxisInfo
{
	MoveTool::MoveToolWidget Widget;
	XMFLOAT3 Direction;
	XMFLOAT3 Color;
};

MoveAxisInfo MoveAxes[] =
{
	{ MoveTool::MoveToolWidget::AxisX, XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.2f, 0.2f) },
	{ MoveTool::MoveToolWidget::AxisY, XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(0.2f, 1.0f, 0.2f) },
	{ MoveTool::MoveToolWidget::AxisZ, XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.2f, 0.2f, 1.0f) },
};

void XM_CALLCONV MoveTool::DrawAxis(const XMVECTOR position, const XMVECTOR direction, const DirectX::XMVECTOR tint) const
{
	float dot = XMVectorGetX(XMVector3Dot(direction, XMVector3Normalize(XMVectorSubtract(this->GetShowcase()->GetCameraController()->GetCameraPosition(), position))));
	if (dot > -0.99f && dot < 0.99f)
	{
		float scale = ARROW_SCALE * this->GetWidgetScale(position);
		XMVECTOR quaternion = XMVectorSetW(XMVector3Cross(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), direction), 1.0f + XMVectorGetX(XMVector3Dot(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), direction)));
		XMMATRIX rotation = XMMatrixRotationQuaternion(XMQuaternionNormalize(quaternion));
		XMMATRIX transform = XMMatrixMultiply(XMMatrixScaling(scale, scale, scale), XMMatrixMultiply(rotation, XMMatrixTranslationFromVector(position)));
		this->GetShowcase()->DrawWidgetMesh(this->_ArrowLineMesh, transform, tint);
		this->GetShowcase()->DrawWidgetMesh(this->_ArrowheadMesh, XMMatrixMultiply(XMMatrixMultiply(XMMatrixScaling(ARROWHEAD_SIZE, ARROWHEAD_SIZE, ARROWHEAD_SIZE), XMMatrixTranslation(0.0f, 0.0f, ARROW_STEM_LENGTH)), transform), tint);
	}
}

bool XM_CALLCONV MoveTool::HitTestAxis(const DirectX::XMVECTOR position, const DirectX::XMVECTOR direction, const DirectX::XMVECTOR rayOrigin, const DirectX::XMVECTOR rayDirection, float& t) const
{
	float scale = this->GetWidgetScale(position) * ARROW_SCALE;
	return IntersectRayCapsule(rayOrigin, rayDirection, position, XMVectorAdd(position, XMVectorScale(direction, (ARROWHEAD_SIZE + ARROW_STEM_LENGTH) * scale)), scale * 0.20f, t);
}

MoveTool::MoveToolWidget MoveTool::HitTestWidgets(int x, int y, XMVECTOR& hitPoint) const
{
	SceneObject* selectedObject = this->GetShowcase()->GetSelectedObject();
	if (selectedObject != nullptr)
	{
		XMVECTOR rayOrigin;
		XMVECTOR rayDirection;
		this->GetScreenRay(x, y, rayOrigin, rayDirection);

		float minDistance = FLT_MAX;
		MoveToolWidget hitWidget = MoveToolWidget::None;

		for (size_t i = 0; i < sizeof(MoveAxes) / sizeof(MoveAxes[0]); i++)
		{
			float distance;
			if (this->HitTestAxis(selectedObject->GetPosition(), XMVector3Rotate(XMLoadFloat3(&MoveAxes[i].Direction), selectedObject->GetRotation()), rayOrigin, rayDirection, distance))
			{
				if (distance < minDistance)
				{
					minDistance = distance;
					hitWidget = MoveAxes[i].Widget;
				}
			}
		}

		// TODO: Plane widgets

		if (hitWidget != MoveToolWidget::None)
		{
			hitPoint = XMVectorAdd(rayOrigin, XMVectorScale(rayDirection, minDistance));
		}
		else
		{
			hitPoint = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
		}

		return hitWidget;
	}
	else
	{
		hitPoint = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
		return MoveToolWidget::None;
	}
}

XMVECTOR XM_CALLCONV MoveTool::SnapToWidget(int x, int y, MoveToolWidget widget) const
{
	SceneObject* selectedObject = this->GetShowcase()->GetSelectedObject();
	if (selectedObject != nullptr)
	{
		XMVECTOR rayOrigin;
		XMVECTOR rayDirection;
		this->GetScreenRay(x, y, rayOrigin, rayDirection);
		if (widget >= MoveToolWidget::AxisX && widget <= MoveToolWidget::AxisZ)
		{
			// Axis widget
			// Project cursor position onto camera-facing plane along the axis and then onto axis
			MoveAxisInfo* axisInfo = &MoveAxes[(int)widget - (int)MoveToolWidget::AxisX];
			XMVECTOR planeNormal = RemoveVectorAxis(this->GetShowcase()->GetCameraController()->GetForwardVector(), XMLoadFloat3(&axisInfo->Direction));
			XMVECTOR pointOnPlane = XMPlaneIntersectLine(XMPlaneFromPointNormal(selectedObject->GetPosition(), planeNormal), rayOrigin, XMVectorAdd(rayOrigin, rayDirection));
			return ProjectPointOntoAxis(pointOnPlane, selectedObject->GetPosition(), XMVector3Rotate(XMLoadFloat3(&axisInfo->Direction), selectedObject->GetRotation()));
		}
		else if (widget >= MoveToolWidget::PlaneXY && widget <= MoveToolWidget::PlaneXZ)
		{
			// Plane widget
			// Project cursor position onto plane

			// TODO: Implement!
			return XMVectorZero();
		}
		else
		{
			// Nobody should be passing in MoveToolWidget::None
			return XMVectorZero();
		}
	}
	else
	{
		// Widgets don't exist while nothing is selected
		return XMVectorZero();
	}
}

MoveTool::MoveTool(Showcase* showcase) : Tool(showcase, "tool_move", ShowcaseIcon_ToolMove)
{
	this->_ArrowheadMesh = CreateArrowheadWidgetMesh(showcase->GetDevice(), XMFLOAT3(1.0f, 1.0f, 1.0f));
	this->_ArrowLineMesh = CreateLineWidgetMesh(showcase->GetDevice(), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, ARROW_STEM_LENGTH), XMFLOAT3(1.0f, 1.0f, 1.0f));
}

MoveTool::~MoveTool()
{
	delete this->_ArrowLineMesh;
	this->_ArrowLineMesh = nullptr;
	delete this->_ArrowheadMesh;
	this->_ArrowheadMesh = nullptr;
}

const char* MoveTool::GetTooltip() const
{
	return "Move Tool";
}

void MoveTool::DrawStatus()
{
	ImGui::Text("Click and drag an axis or plane to move the selected object.");
}

void MoveTool::DrawWidgets()
{
	Showcase* showcase = this->GetShowcase();
	if (showcase->GetSelectedObject() != nullptr)
	{
		for (size_t i = 0; i < sizeof(MoveAxes) / sizeof(MoveAxes[0]); i++)
		{
			XMVECTOR color = XMVectorSetW(XMLoadFloat3(&MoveAxes[i].Color), 1.0f);
			if (this->_ActiveWidget == MoveAxes[i].Widget)
			{
				color = XMVectorLerp(color, XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f), this->_IsDraggingWidget ? 1.0f : 0.55f);
			}
			this->DrawAxis(showcase->GetSelectedObject()->GetPosition(), XMVector3Rotate(XMLoadFloat3(&MoveAxes[i].Direction), showcase->GetSelectedObject()->GetRotation()), color);
		}
	}
}

void MoveTool::OnMouseDown(int button, int x, int y)
{
	SceneObject* selectedObject = this->GetShowcase()->GetSelectedObject();
	if (selectedObject != nullptr)
	{
		XMVECTOR hitPoint;

		if (button == MOUSE_BUTTON_LEFT)
		{
			this->_ActiveWidget = this->HitTestWidgets(x, y, hitPoint);
			if (this->_ActiveWidget != MoveToolWidget::None)
			{
				// Begin the drag
				this->_IsDraggingWidget = true;
				XMStoreFloat3(&this->_DragInitialPosition, selectedObject->GetPosition());
				XMStoreFloat3(&this->_DragInitialCursor, this->SnapToWidget(x, y, this->_ActiveWidget));
			}
		}
	}

}

void MoveTool::OnMouseUp(int button, int x, int y)
{
	if (button == MOUSE_BUTTON_LEFT && this->_IsDraggingWidget)
	{
		this->_IsDraggingWidget = false;
	}
}

void MoveTool::OnMouseMove(int x, int y)
{
	if (!this->_IsDraggingWidget)
	{
		XMVECTOR hitPoint;
		this->_ActiveWidget = this->HitTestWidgets(x, y, hitPoint);
	}
	else
	{
		SceneObject* selectedObject = this->GetShowcase()->GetSelectedObject();
		if (selectedObject != nullptr)
		{
			// Update object transform
			XMVECTOR delta = XMVectorSubtract(this->SnapToWidget(x, y, this->_ActiveWidget), XMLoadFloat3(&this->_DragInitialCursor));
			selectedObject->SetPosition(XMVectorAdd(XMLoadFloat3(&this->_DragInitialPosition), delta));
		}
	}
}
