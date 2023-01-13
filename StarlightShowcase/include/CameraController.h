#pragma once

#include "DirectXMath.h"

/**
 * A combination first- and third-person camera controller, UnrealEd style.
 */
class CameraController
{
private:
	// Authoritative first-person values
	DirectX::XMFLOAT3 _Position = DirectX::XMFLOAT3(5.0f, 1.0f, 5.0f);
	float _Pitch = -0.4f;
	float _Yaw = 3.0f * 3.14159265f / 4.0f;

	// Third-person values
	float _Distance;
	//DirectX::XMFLOAT3 _Center;

	float _MovementSpeed = 3.0f;
	float _RotationSensitivity = 0.004f; // Radians per pixel

	// Input state
	bool _FirstPersonLooking : 1;
	bool _ThirdPersonKeyActive : 1;
	bool _StrafeForwardPressed : 1;
	bool _StrafeBackwardPressed : 1;
	bool _StrafeLeftPressed : 1;
	bool _StrafeRightPressed : 1;
	bool _StrafeUpPressed : 1;
	bool _StrafeDownPressed : 1;
	bool _OrbitActive : 1;
	bool _PanActive : 1;
	bool _ZoomActive : 1;

public:
	
	inline DirectX::XMVECTOR XM_CALLCONV GetCameraPosition() const { return DirectX::XMLoadFloat3(&this->_Position); }
	inline DirectX::XMVECTOR XM_CALLCONV GetCameraRotation() const { return DirectX::XMQuaternionMultiply(DirectX::XMQuaternionRotationAxis(DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), -this->_Pitch), DirectX::XMQuaternionRotationAxis(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), this->_Yaw)); }

	void XM_CALLCONV SetViewFirstPerson(const DirectX::XMVECTOR position, float pitch, float yaw);
	//void XM_CALLCONV SetViewOrbit(const DirectX::XMVECTOR center, float distance, float pitch, float yaw);

	void Update(float deltaTime);

	bool KeyDown(int virtualKey);
	bool KeyUp(int virtualKey);

	bool MouseDown(int button);
	bool MouseUp(int button);
	bool MouseMove(float dx, float dy);
};