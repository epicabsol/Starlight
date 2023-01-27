#include "tools/Tool.h"

#include "Showcase.h"
#include "render/DeferredPipeline.h"

using namespace DirectX;

Tool::Tool(Showcase* showcase, const char* id, ShowcaseIcon icon) : _Showcase(showcase), _ID(id), _Icon(icon)
{

}

Tool::~Tool()
{

}

void XM_CALLCONV Tool::GetScreenRay(int x, int y, DirectX::XMVECTOR& rayOrigin, DirectX::XMVECTOR& rayDirection) const
{
	float relativeX = ((float)x / this->GetShowcase()->GetPipeline()->GetWidth()) * 2.0f - 1.0f;
	float relativeY = ((float)y / this->GetShowcase()->GetPipeline()->GetHeight()) * -2.0f + 1.0f;

	XMMATRIX transform = XMMatrixMultiply(this->GetShowcase()->GetCamera()->GetInverseProjectionMatrix(), this->GetShowcase()->GetCamera()->GetInverseViewMatrix());

	XMVECTOR nearPoint = XMVectorSet(relativeX, relativeY, 0.0f, 1.0f);
	rayOrigin = XMVector3TransformCoord(nearPoint, transform);

	XMVECTOR farPoint = XMVectorSet(relativeX, relativeY, 1.0f, 1.0f);
	farPoint = XMVector3TransformCoord(farPoint, transform);
	rayDirection = XMVector3Normalize(XMVectorSubtract(farPoint, rayOrigin));
}

void Tool::Activate()
{
	this->_IsActive = true;
}

void Tool::Deactivate()
{
	this->_IsActive = false;
}

void Tool::DrawStatus()
{

}

void Tool::DrawWidgets()
{

}

void Tool::OnKeyDown(int virtualKey)
{

}

void Tool::OnKeyUp(int virtualKey)
{

}

void Tool::OnMouseDown(int button, int x, int y)
{

}

void Tool::OnMouseUp(int button, int x, int y)
{

}

void Tool::OnMouseMove(int x, int y)
{

}
