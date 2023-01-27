#pragma once

#include "DirectXMath.h"

class Camera
{
private:
	DirectX::XMFLOAT3 _Position = DirectX::XMFLOAT3();
	DirectX::XMFLOAT4 _Rotation = DirectX::XMFLOAT4();

	float _VerticalFOV;
	float _AspectRatio;
	float _MinDepth;
	float _MaxDepth;

	mutable DirectX::XMFLOAT4X4 _ViewMatrix = DirectX::XMFLOAT4X4();
	mutable DirectX::XMFLOAT4X4 _InverseViewMatrix = DirectX::XMFLOAT4X4();
	mutable DirectX::XMFLOAT4X4 _ProjectionMatrix = DirectX::XMFLOAT4X4();
	mutable DirectX::XMFLOAT4X4 _InverseProjectionMatrix = DirectX::XMFLOAT4X4();
	mutable bool _ViewMatrixStale = true;
	mutable bool _ProjectionMatrixStale = true;

	DirectX::XMMATRIX XM_CALLCONV RecalculateViewMatrix() const;
	DirectX::XMMATRIX XM_CALLCONV RecalculateProjectionMatrix() const;

public:
	Camera(float aspectRatio = 1.0f, float verticalFOV = 1.0f, float minDepth = 0.25f, float maxDepth = 1000.0f);

	inline DirectX::XMVECTOR XM_CALLCONV GetPosition() const { return XMLoadFloat3(&this->_Position); }
	inline void XM_CALLCONV SetPosition(const DirectX::XMVECTOR position) { DirectX::XMStoreFloat3(&this->_Position, position); this->_ViewMatrixStale = true; }
	inline DirectX::XMVECTOR XM_CALLCONV GetRotation() const { return XMLoadFloat4(&this->_Rotation); }
	inline void XM_CALLCONV SetRotation(const DirectX::XMVECTOR rotation) { DirectX::XMStoreFloat4(&this->_Rotation, rotation); this->_ViewMatrixStale = true; }

	inline float GetVerticalFOV() const { return this->_VerticalFOV; }
	inline void SetVerticalFOV(float verticalFOV) { this->_VerticalFOV = verticalFOV; this->_ProjectionMatrixStale = true; }
	inline float GetAspectRatio() const { return this->_AspectRatio; }
	inline void SetAspectRatio(float aspectRatio) { this->_AspectRatio = aspectRatio; this->_ProjectionMatrixStale = true; }

	inline DirectX::XMMATRIX XM_CALLCONV GetViewMatrix() const
	{
		if (this->_ViewMatrixStale)
		{
			return this->RecalculateViewMatrix();
		}
		else
		{
			return DirectX::XMLoadFloat4x4(&this->_ViewMatrix);
		}
	}

	inline DirectX::XMMATRIX XM_CALLCONV GetInverseViewMatrix() const
	{
		if (this->_ViewMatrixStale)
		{
			this->RecalculateViewMatrix();
		}
		return DirectX::XMLoadFloat4x4(&this->_InverseViewMatrix);
	}

	inline DirectX::XMMATRIX XM_CALLCONV GetProjectionMatrix() const
	{
		if (this->_ProjectionMatrixStale)
		{
			return this->RecalculateProjectionMatrix();
		}
		else
		{
			return DirectX::XMLoadFloat4x4(&this->_ProjectionMatrix);
		}
	}

	inline DirectX::XMMATRIX XM_CALLCONV GetInverseProjectionMatrix() const
	{
		if (this->_ProjectionMatrixStale)
		{
			this->RecalculateProjectionMatrix();
		}
		return XMLoadFloat4x4(&this->_InverseProjectionMatrix);
	}
};