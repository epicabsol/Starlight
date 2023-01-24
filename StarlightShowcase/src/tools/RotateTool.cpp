#include "tools/RotateTool.h"

#include "imgui/imgui.h"

#include "Showcase.h"
#include "Widget.h"

using namespace DirectX;

void XM_CALLCONV RotateTool::DrawCircle(const XMVECTOR position, const XMVECTOR direction, const XMFLOAT4& tint) const
{
	float dot = XMVectorGetX(XMVector3Dot(direction, XMVector3Normalize(XMVectorSubtract(this->GetShowcase()->GetCameraController()->GetCameraPosition(), position))));
	if (dot > 0.03f || dot < -0.03f)
	{
		float scale = 0.1f * XMVectorGetZ(XMVector3TransformCoord(position, this->GetShowcase()->GetCamera()->GetViewMatrix()));
		XMMATRIX transform = XMMatrixMultiply(XMMatrixScaling(scale, scale, scale), XMMatrixMultiply(XMMatrixTranslationFromVector(position), XMVectorGetY(direction) > 0.999f ? XMMatrixRotationX(-3.14159265f / 2.0f) : XMMatrixLookToLH(XMVectorZero(), direction, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f))));
		this->GetShowcase()->DrawWidgetMesh(this->_CircleMesh, transform, XMLoadFloat4(&tint));
	}
}

RotateTool::RotateTool(Showcase* showcase) : Tool(showcase, "tool_rotate", ShowcaseIcon::ShowcaseIcon_ToolRotate)
{
	this->_CircleMesh = CreateCircleWidgetMesh(showcase->GetDevice(), 1.0f, XMFLOAT3(1.0f, 1.0f, 1.0f));
}

RotateTool::~RotateTool()
{

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
	this->DrawCircle(XMVectorZero(), XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), XMFLOAT4(1.0f, 0.2f, 0.2f, 1.0f));
	this->DrawCircle(XMVectorZero(), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), XMFLOAT4(0.2f, 1.0f, 0.2f, 1.0f));
	this->DrawCircle(XMVectorZero(), XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), XMFLOAT4(0.2f, 0.2f, 1.0f, 1.0f));
}

void RotateTool::OnMouseDown(int button, int x, int y)
{
	// TODO: Implement!
}

void RotateTool::OnMouseUp(int button, int x, int y)
{
	// TODO: Implement!
}

void RotateTool::OnMouseMove(int x, int y)
{
	// TODO: Implement!
}