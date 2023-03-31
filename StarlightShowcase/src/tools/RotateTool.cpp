#include "tools/RotateTool.h"

#include "imgui/imgui.h"

#include "Showcase.h"
#include "Widget.h"
#include "scene/SceneObject.h"
#include "Intersection.h"

using namespace DirectX;

#define DISC_SCALE (0.1f)
#define DISC_RADIUS (1.0f)
#define DISC_THICKNESS (0.05f)

struct RotateDiscInfo
{
	RotateTool::RotateToolWidget Widget;
	XMFLOAT3 Direction;
	XMFLOAT3 Color;
};

RotateDiscInfo RotateDiscs[] =
{
	{ RotateTool::RotateToolWidget::DiscX, XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.2f, 0.2f) },
	{ RotateTool::RotateToolWidget::DiscY, XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(0.2f, 1.0f, 0.2f) },
	{ RotateTool::RotateToolWidget::DiscZ, XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.2f, 0.2f, 1.0f) },
};

void XM_CALLCONV RotateTool::DrawDisc(const XMVECTOR position, const XMVECTOR direction, const XMVECTOR tint, float scale) const
{
	XMVECTOR toCamera = XMVectorSubtract(this->GetShowcase()->GetCameraController()->GetCameraPosition(), position);
	float dot = XMVectorGetX(XMVector3Dot(direction, XMVector3Normalize(toCamera)));
	if (dot > 0.03f || dot < -0.03f)
	{
		scale = scale * DISC_SCALE * this->GetWidgetScale(position);
		XMVECTOR quaternion = XMVectorSetW(XMVector3Cross(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), direction), 1.0f + XMVectorGetX(XMVector3Dot(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), direction)));
		XMMATRIX rotation = XMMatrixRotationQuaternion(XMQuaternionNormalize(quaternion));
		XMMATRIX transform = XMMatrixMultiply(XMMatrixScaling(scale, scale, scale), XMMatrixMultiply(rotation, XMMatrixTranslationFromVector(position)));
		this->GetShowcase()->DrawWidgetMesh(this->_CircleMesh, transform, tint);
		this->GetShowcase()->DrawWidgetMesh(this->_DiscMesh, transform, tint);
	}
}

bool XM_CALLCONV RotateTool::HitTestDisc(const XMVECTOR position, const XMVECTOR direction, const XMVECTOR rayOrigin, const XMVECTOR rayDirection, float& t) const
{
	return IntersectRayDisc(rayOrigin, rayDirection, position, direction, DISC_RADIUS * DISC_SCALE * this->GetWidgetScale(position), (DISC_THICKNESS + 0.025f) * DISC_SCALE * this->GetWidgetScale(position), t);
}

RotateTool::RotateToolWidget RotateTool::HitTestWidgets(int x, int y, XMVECTOR& hitPoint) const
{
	SceneObject* selectedObject = this->GetShowcase()->GetSelectedObject();
	if (selectedObject != nullptr)
	{
		XMVECTOR rayOrigin;
		XMVECTOR rayDirection;
		this->GetScreenRay(x, y, rayOrigin, rayDirection);

		float minDistance = FLT_MAX;
		RotateToolWidget hitWidget = RotateToolWidget::None;

		for (size_t i = 0; i < sizeof(RotateDiscs) / sizeof(RotateDiscs[0]); i++)
		{
			float distance;
			if (this->HitTestDisc(selectedObject->GetPosition(), XMVector3Rotate(XMLoadFloat3(&RotateDiscs[i].Direction), selectedObject->GetRotation()), rayOrigin, rayDirection, distance))
			{
				if (distance < minDistance)
				{
					minDistance = distance;
					hitWidget = RotateDiscs[i].Widget;
				}
			}
		}

		if (hitWidget != RotateToolWidget::None)
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
		return RotateToolWidget::None;
	}
}

XMVECTOR XM_CALLCONV RotateTool::SnapToWidget(int x, int y, RotateToolWidget widget) const
{
	SceneObject* selectedObject = this->GetShowcase()->GetSelectedObject();
	if (selectedObject != nullptr)
	{
		XMVECTOR rayOrigin;
		XMVECTOR rayDirection;
		this->GetScreenRay(x, y, rayOrigin, rayDirection);
		if (widget >= RotateToolWidget::DiscX && widget <= RotateToolWidget::DiscZ)
		{
			XMVECTOR point = selectedObject->GetPosition();
			XMVECTOR normal = XMVector3Rotate(XMLoadFloat3(&RotateDiscs[(int)widget - (int)RotateToolWidget::DiscX].Direction), selectedObject->GetRotation());
			return XMPlaneIntersectLine(XMPlaneFromPointNormal(point, normal), rayOrigin, XMVectorAdd(rayOrigin, rayDirection));
		}
		else
		{
			return XMVectorZero();
		}
	}
	else
	{
		return XMVectorZero();
	}
}

RotateTool::RotateTool(Showcase* showcase) : Tool(showcase, "tool_rotate", ShowcaseIcon::ShowcaseIcon_ToolRotate)
{
	this->_CircleMesh = CreateCircleWidgetMesh(showcase->GetDevice(), DISC_RADIUS, XMFLOAT3(1.0f, 1.0f, 1.0f));
	this->_DiscMesh = CreateDiscWidgetMesh(showcase->GetDevice(), DISC_RADIUS, DISC_THICKNESS, XMFLOAT3(1.0f, 1.0f, 1.0f));
}

RotateTool::~RotateTool()
{
	delete this->_DiscMesh;
	this->_DiscMesh = nullptr;
	delete this->_CircleMesh;
	this->_CircleMesh = nullptr;
}

const char* RotateTool::GetTooltip() const
{
	return "Rotate Tool";
}

void RotateTool::DrawStatus()
{
	ImGui::Text("Click and drag a disc to rotate the selected object.");
}

void RotateTool::DrawWidgets()
{
	SceneObject* selectedObject = this->GetShowcase()->GetSelectedObject();
	if (selectedObject != nullptr)
	{
		for (size_t i = 0; i < sizeof(RotateDiscs) / sizeof(RotateDiscs[0]); i++)
		{
			XMVECTOR color = XMVectorSetW(XMLoadFloat3(&RotateDiscs[i].Color), 1.0f);
			if (this->_ActiveWidget == RotateDiscs[i].Widget)
			{
				color = XMVectorLerp(color, XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f), this->_IsDraggingWidget ? 1.0f : 0.55f);
			}
			this->DrawDisc(selectedObject->GetPosition(), XMVector3Rotate(XMLoadFloat3(&RotateDiscs[i].Direction), selectedObject->GetRotation()), color, 1.0f);
		}
		this->DrawDisc(selectedObject->GetPosition(), XMVector3Normalize(XMVectorSubtract(this->GetShowcase()->GetCamera()->GetPosition(), selectedObject->GetPosition())), XMVectorSet(0.5f, 0.5f, 0.5f, 1.0f), 1.25f);
	}
}

void RotateTool::OnMouseDown(int button, int x, int y)
{
	SceneObject* selectedObject = this->GetShowcase()->GetSelectedObject();
	if (selectedObject != nullptr)
	{
		XMVECTOR hitPoint;

		if (button == MOUSE_BUTTON_LEFT)
		{
			this->_ActiveWidget = this->HitTestWidgets(x, y, hitPoint);
			if (this->_ActiveWidget != RotateToolWidget::None)
			{
				// Begin the drag
				this->_IsDraggingWidget = true;
				XMStoreFloat4(&this->_DragInitialRotation, selectedObject->GetRotation());
				XMStoreFloat3(&this->_DragInitialCursor, this->SnapToWidget(x, y, this->_ActiveWidget));
			}
		}
	}
}

void RotateTool::OnMouseUp(int button, int x, int y)
{
	if (button == MOUSE_BUTTON_LEFT && this->_IsDraggingWidget)
	{
		this->_IsDraggingWidget = false;
	}
}

void RotateTool::OnMouseMove(int x, int y)
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
			// Update object rotation
			XMVECTOR originalDirection = XMVector3Normalize(XMVectorSubtract(XMLoadFloat3(&this->_DragInitialCursor), selectedObject->GetPosition()));
			XMVECTOR newDirection = XMVector3Normalize(XMVectorSubtract(this->SnapToWidget(x, y, this->_ActiveWidget), selectedObject->GetPosition()));
			XMVECTOR discNormal = XMVector3Rotate(XMLoadFloat3(&RotateDiscs[(int)this->_ActiveWidget - (int)RotateToolWidget::DiscX].Direction), XMLoadFloat4(&this->_DragInitialRotation));

			float dot = XMVectorGetX(XMVector3Dot(originalDirection, newDirection));
			float angle = acosf(dot);

			XMVECTOR orthogonal = XMVector3Normalize(XMVector3Cross(discNormal, originalDirection));
			angle = copysignf(angle, XMVectorGetX(XMVector3Dot(orthogonal, newDirection)));

			selectedObject->SetRotation(XMQuaternionNormalize(XMQuaternionMultiply(XMLoadFloat4(&this->_DragInitialRotation), XMQuaternionRotationAxis(discNormal, angle))));
		}
	}
}