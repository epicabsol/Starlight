#include "Camera.h"

using namespace DirectX;

XMMATRIX XM_CALLCONV Camera::RecalculateViewMatrix() const
{
	XMMATRIX view = XMMatrixMultiply(XMMatrixTranslationFromVector(XMVectorNegate(this->GetPosition())), XMMatrixRotationQuaternion(XMQuaternionInverse(this->GetRotation())));
	XMStoreFloat4x4(&this->_ViewMatrix, view);
	XMStoreFloat4x4(&this->_InverseViewMatrix, XMMatrixInverse(nullptr, view));
	this->_ViewMatrixStale = false;
	return view;
}

XMMATRIX XM_CALLCONV Camera::RecalculateProjectionMatrix() const
{
	XMMATRIX projection = XMMatrixPerspectiveFovLH(this->_VerticalFOV, this->_AspectRatio, this->_MinDepth, this->_MaxDepth);
	XMStoreFloat4x4(&this->_ProjectionMatrix, projection);
	XMStoreFloat4x4(&this->_InverseProjectionMatrix, XMMatrixInverse(nullptr, projection));
	this->_ProjectionMatrixStale = false;
	return projection;
}

Camera::Camera(float aspectRatio, float verticalFOV, float minDepth, float maxDepth)
	: _VerticalFOV(verticalFOV), _AspectRatio(aspectRatio), _MinDepth(minDepth), _MaxDepth(maxDepth)
{
	XMStoreFloat4(&this->_Rotation, XMQuaternionIdentity());
}