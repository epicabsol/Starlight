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

void XM_CALLCONV MoveTool::DrawAxis(const XMVECTOR position, const XMVECTOR direction, const DirectX::XMFLOAT4& tint) const
{
	float dot = XMVectorGetX(XMVector3Dot(direction, XMVector3Normalize(XMVectorSubtract(this->GetShowcase()->GetCameraController()->GetCameraPosition(), position))));
	if (dot > -0.99f && dot < 0.99f)
	{
		float scale = ARROW_SCALE * this->GetWidgetScale(position);
		XMMATRIX rotation = XMVectorGetY(direction) > 0.999f ? XMMatrixRotationX(-3.14159265f / 2.0f) : XMMatrixLookToLH(XMVectorZero(), XMVectorMultiply(direction, XMVectorSet(-1.0f, 1.0f, 1.0f, 1.0f)), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
		XMMATRIX transform = XMMatrixMultiply(XMMatrixScaling(scale, scale, scale), XMMatrixMultiply(rotation, XMMatrixTranslationFromVector(position)));
		this->GetShowcase()->DrawWidgetMesh(this->_ArrowLineMesh, transform, XMLoadFloat4(&tint));
		this->GetShowcase()->DrawWidgetMesh(this->_ArrowheadMesh, XMMatrixMultiply(XMMatrixMultiply(XMMatrixScaling(ARROWHEAD_SIZE, ARROWHEAD_SIZE, ARROWHEAD_SIZE), XMMatrixTranslation(0.0f, 0.0f, ARROW_STEM_LENGTH)), transform), XMLoadFloat4(&tint));
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
			if (this->HitTestAxis(selectedObject->GetPosition(), XMLoadFloat3(&MoveAxes[i].Direction), rayOrigin, rayDirection, distance))
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
			XMFLOAT4 color = (MoveAxes[i].Widget == this->_ActiveWidget) ? XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) : XMFLOAT4(MoveAxes[i].Color.x, MoveAxes[i].Color.y, MoveAxes[i].Color.z, 1.0f);
			this->DrawAxis(showcase->GetSelectedObject()->GetPosition(), XMVector3Rotate(XMLoadFloat3(&MoveAxes[i].Direction), showcase->GetSelectedObject()->GetRotation()), color);
		}
	}
}

void MoveTool::OnMouseDown(int button, int x, int y)
{
	// TODO: Implement!
}

void MoveTool::OnMouseUp(int button, int x, int y)
{
	// TODO: Implement!
}

void MoveTool::OnMouseMove(int x, int y)
{
	XMVECTOR hitPoint;
	this->_ActiveWidget = this->HitTestWidgets(x, y, hitPoint);
}
