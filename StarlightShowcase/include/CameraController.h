#pragma once

#include "DirectXMath.h"

/**
 * A combination first- and third-person camera controller, UnrealEd style.
 */
class CameraController
{
private:
	// First-person values
	DirectX::XMFLOAT3 _Position = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	float _Pitch = 0.0f;
	float _Yaw = 0.0f;

	// Third-person values
	float _Distance = 5.0f;

	// Input configuration
	float _MovementSpeed = 3.0f;
	float _SprintSpeedFactor = 3.0f;
	float _RotationSensitivity = 0.004f; // Radians per pixel
	float _TranslationSensitivity = 0.002f; // Units per pixel

	// Input state
	bool _FirstPersonLooking : 1;
	bool _ThirdPersonKeyActive : 1;
	bool _StrafeForwardPressed : 1;
	bool _StrafeBackwardPressed : 1;
	bool _StrafeLeftPressed : 1;
	bool _StrafeRightPressed : 1;
	bool _StrafeUpPressed : 1;
	bool _StrafeDownPressed : 1;
	bool _StrafeSprintPressed : 1;
	bool _OrbitActive : 1;
	bool _PanActive : 1;
	bool _ZoomActive : 1;

public:
	
	inline DirectX::XMVECTOR XM_CALLCONV GetCameraPosition() const { return DirectX::XMLoadFloat3(&this->_Position); }
	inline DirectX::XMVECTOR XM_CALLCONV GetCameraRotation() const { return DirectX::XMQuaternionMultiply(DirectX::XMQuaternionRotationAxis(DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), -this->_Pitch), DirectX::XMQuaternionRotationAxis(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), this->_Yaw)); }
	inline DirectX::XMVECTOR XM_CALLCONV GetForwardVector() const { return DirectX::XMVector3Rotate(DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), this->GetCameraRotation()); };
	inline DirectX::XMVECTOR XM_CALLCONV GetRightVector() const { return DirectX::XMVector3Rotate(DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), this->GetCameraRotation()); }
	inline DirectX::XMVECTOR XM_CALLCONV GetUpVector() const { return DirectX::XMVector3Rotate(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), this->GetCameraRotation()); }

	void XM_CALLCONV SetViewFirstPerson(const DirectX::XMVECTOR position, float pitch, float yaw);
	void XM_CALLCONV SetViewThirdPerson(const DirectX::XMVECTOR center, float distance, float pitch, float yaw);

	void Update(float deltaTime);

	bool KeyDown(int virtualKey);
	bool KeyUp(int virtualKey);

	bool MouseDown(int button);
	bool MouseUp(int button);
	bool MouseMove(float dx, float dy);
};