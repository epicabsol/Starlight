#include "CameraController.h"

#include "Window.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d11.h>

using namespace DirectX;

#define CAMERACONTROLLER_MINIMUM_ZOOM (0.05f)

void XM_CALLCONV CameraController::SetViewFirstPerson(const XMVECTOR position, float pitch, float yaw)
{
	XMStoreFloat3(&this->_Position, position);
	this->_Pitch = pitch;
	this->_Yaw = yaw;
}

void XM_CALLCONV CameraController::SetViewThirdPerson(const XMVECTOR center, float distance, float pitch, float yaw)
{
	this->_Pitch = pitch;
	this->_Yaw = yaw;
	this->_Distance = distance;
	XMStoreFloat3(&this->_Position, XMVectorAdd(center, XMVectorScale(this->GetForwardVector(), -distance)));
}

void CameraController::Update(float deltaTime)
{
	XMVECTOR movement = XMVectorZero();
	XMVECTOR rotation = this->GetCameraRotation();
	float speed = this->_MovementSpeed * (this->_StrafeSprintPressed ? this->_SprintSpeedFactor : 1.0f);
	XMVECTOR forwards = this->GetForwardVector();
	XMVECTOR right = this->GetRightVector();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	if (this->_StrafeUpPressed)
	{
		movement = XMVectorAdd(movement, up);
	}
	
	if (this->_StrafeDownPressed)
	{
		movement = XMVectorAdd(movement, XMVectorNegate(up));
	}

	if (this->_StrafeForwardPressed)
	{
		movement = XMVectorAdd(movement, forwards);
	}

	if (this->_StrafeBackwardPressed)
	{
		movement = XMVectorAdd(movement, XMVectorNegate(forwards));
	}

	if (this->_StrafeRightPressed)
	{
		movement = XMVectorAdd(movement, right);
	}

	if (this->_StrafeLeftPressed)
	{
		movement = XMVectorAdd(movement, XMVectorNegate(right));
	}

	movement = XMVector3Normalize(movement);

	XMStoreFloat3(&this->_Position, XMVectorAdd(this->GetCameraPosition(), XMVectorScale(movement, speed * deltaTime)));
}

bool CameraController::KeyDown(int virtualKey)
{
	if (this->_FirstPersonLooking)
	{
		if (virtualKey == 'E')
		{
			this->_StrafeUpPressed = true;
			return true;
		}
		else if (virtualKey == 'Q')
		{
			this->_StrafeDownPressed = true;
			return true;
		}
		else if (virtualKey == 'W')
		{
			this->_StrafeForwardPressed = true;
			return true;
		}
		else if (virtualKey == 'S')
		{
			this->_StrafeBackwardPressed = true;
			return true;
		}
		else if (virtualKey == 'A')
		{
			this->_StrafeLeftPressed = true;
			return true;
		}
		else if (virtualKey == 'D')
		{
			this->_StrafeRightPressed = true;
			return true;
		}
		else if (virtualKey == VK_SHIFT)
		{
			this->_StrafeSprintPressed = true;
			return true;
		}
	}
	else if (virtualKey == VK_MENU)
	{
		this->_ThirdPersonKeyActive = true;
		return true;
	}
}

bool CameraController::KeyUp(int virtualKey)
{
	if (virtualKey == 'E')
	{
		this->_StrafeUpPressed = false;
		return true;
	}
	else if (virtualKey == 'Q')
	{
		this->_StrafeDownPressed = false;
		return true;
	}
	else if (virtualKey == 'W')
	{
		this->_StrafeForwardPressed = false;
		return true;
	}
	else if (virtualKey == 'S')
	{
		this->_StrafeBackwardPressed = false;
		return true;
	}
	else if (virtualKey == 'A')
	{
		this->_StrafeLeftPressed = false;
		return true;
	}
	else if (virtualKey == 'D')
	{
		this->_StrafeRightPressed = false;
		return true;
	}
	else if (virtualKey == VK_MENU)
	{
		this->_ThirdPersonKeyActive = false;
		return true;
	}
	else if (virtualKey == VK_SHIFT)
	{
		this->_StrafeSprintPressed = false;
		return true;
	}
}

bool CameraController::MouseDown(int button)
{
	if (this->_ThirdPersonKeyActive)
	{
		if (button == MOUSE_BUTTON_LEFT)
		{
			this->_OrbitActive = true;
		}
		else if (button == MOUSE_BUTTON_RIGHT)
		{
			this->_ZoomActive = true;
		}
		else if (button == MOUSE_BUTTON_MIDDLE)
		{
			this->_PanActive = true;
		}
		return true;
	}
	else if (!this->_OrbitActive && !this->_PanActive && !this->_ZoomActive && button == MOUSE_BUTTON_RIGHT)
	{
		this->_FirstPersonLooking = true;
		return true;
	}
	else
	{
		return false;
	}
}

bool CameraController::MouseUp(int button)
{
	if (button == MOUSE_BUTTON_LEFT && this->_OrbitActive)
	{
		this->_OrbitActive = false;
		return true;
	}
	else if (button == MOUSE_BUTTON_MIDDLE && this->_PanActive)
	{
		this->_PanActive = false;
		return true;
	}
	else if (button == MOUSE_BUTTON_RIGHT && this->_ZoomActive)
	{
		this->_ZoomActive = false;
		return true;
	}
	else if (button == MOUSE_BUTTON_RIGHT && this->_FirstPersonLooking)
	{
		this->_FirstPersonLooking = false;
		return true;
	}
	else
	{
		return false;
	}
}

bool CameraController::MouseMove(float dx, float dy)
{
	if (this->_FirstPersonLooking)
	{
		this->_Pitch -= dy * this->_RotationSensitivity;
		this->_Yaw += dx * this->_RotationSensitivity;
		return true;
	}
	else if (this->_OrbitActive)
	{
		XMVECTOR forward = XMVectorScale(this->GetForwardVector(), this->_Distance);
		XMVECTOR focusPoint = XMVectorAdd(this->GetCameraPosition(), forward);
		this->_Pitch -= dy * this->_RotationSensitivity;
		this->_Yaw += dx * this->_RotationSensitivity;
		forward = XMVectorScale(this->GetForwardVector(), this->_Distance);
		XMStoreFloat3(&this->_Position, XMVectorSubtract(focusPoint, forward));
		return true;
	}
	else if (this->_PanActive)
	{
		XMVECTOR right = XMVectorScale(this->GetRightVector(), -this->_TranslationSensitivity * this->_Distance * dx);
		XMVECTOR up = XMVectorScale(this->GetUpVector(), this->_TranslationSensitivity * this->_Distance * dy);
		XMStoreFloat3(&this->_Position, XMVectorAdd(XMVectorAdd(this->GetCameraPosition(), right), up));
		return true;
	}
	else if (this->_ZoomActive)
	{
		XMVECTOR forward = this->GetForwardVector();
		XMVECTOR center = XMVectorAdd(this->GetCameraPosition(), XMVectorScale(forward, this->_Distance));
		this->_Distance = this->_Distance + dy * this->_Distance * 0.01f;
		if (this->_Distance < CAMERACONTROLLER_MINIMUM_ZOOM)
		{
			this->_Distance = CAMERACONTROLLER_MINIMUM_ZOOM;
		}
		XMStoreFloat3(&this->_Position, XMVectorAdd(center, XMVectorScale(forward, -this->_Distance)));
		return true;
	}
	else
	{
		return false;
	}
}