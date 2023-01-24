#include "tools/MoveTool.h"

#include "imgui/imgui.h"

#include "render/Mesh.h"
#include "Showcase.h"
#include "Widget.h"
#include "scene/SceneObject.h"

using namespace DirectX;

#define ARROW_STEM_LENGTH (1.0f)

void XM_CALLCONV MoveTool::DrawAxis(const XMVECTOR position, const XMVECTOR direction, const DirectX::XMFLOAT4& tint) const
{
	float dot = XMVectorGetX(XMVector3Dot(direction, XMVector3Normalize(XMVectorSubtract(this->GetShowcase()->GetCameraController()->GetCameraPosition(), position))));
	if (dot > -0.99f && dot < 0.99f)
	{
		float scale = 0.1f * XMVectorGetZ(XMVector3TransformCoord(position, this->GetShowcase()->GetCamera()->GetViewMatrix()));
		XMMATRIX transform = XMMatrixMultiply(XMMatrixScaling(scale, scale, scale), XMMatrixMultiply(XMMatrixTranslationFromVector(position), XMVectorGetY(direction) > 0.999f ? XMMatrixRotationX(-3.14159265f / 2.0f) : XMMatrixLookToLH(XMVectorZero(), direction, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f))));
		this->GetShowcase()->DrawWidgetMesh(this->_ArrowLineMesh, transform, XMLoadFloat4(&tint));
		this->GetShowcase()->DrawWidgetMesh(this->_ArrowheadMesh, XMMatrixMultiply(XMMatrixMultiply(XMMatrixScaling(0.35f, 0.35f, 0.35f), XMMatrixTranslation(0.0f, 0.0f, ARROW_STEM_LENGTH)), transform), XMLoadFloat4(&tint));
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
		this->DrawAxis(showcase->GetSelectedObject()->GetPosition(), XMVector3Rotate(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), showcase->GetSelectedObject()->GetRotation()), XMFLOAT4(1.0f, 0.2f, 0.2f, 1.0f));
		this->DrawAxis(showcase->GetSelectedObject()->GetPosition(), XMVector3Rotate(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), showcase->GetSelectedObject()->GetRotation()), XMFLOAT4(0.2f, 1.0f, 0.2f, 1.0f));
		this->DrawAxis(showcase->GetSelectedObject()->GetPosition(), XMVector3Rotate(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), showcase->GetSelectedObject()->GetRotation()), XMFLOAT4(0.2f, 0.2f, 1.0f, 1.0f));
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
	// TODO: Implement!
}
