#include "CameraController.h"

#include "Window.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d11.h>

using namespace DirectX;

#define VK_LEFTCTRL (162)
#define VK_RIGHTCTRL (163)
#define VK_LEFTALT (164)
#define VK_RIGHTALT (165)

void XM_CALLCONV CameraController::SetViewFirstPerson(const XMVECTOR position, float pitch, float yaw)
{
	XMStoreFloat3(&this->_Position, position);
	this->_Pitch = pitch;
	this->_Yaw = yaw;
}

void CameraController::Update(float deltaTime)
{
	XMVECTOR position = this->GetCameraPosition();
	XMVECTOR rotation = this->GetCameraRotation();
	XMVECTOR forwards = XMVector3Rotate(XMVectorSet(0.0f, 0.0f, this->_MovementSpeed * deltaTime, 0.0f), rotation);
	XMVECTOR right = XMVector3Rotate(XMVectorSet(this->_MovementSpeed * deltaTime, 0.0f, 0.0f, 0.0f), rotation);

	if (this->_StrafeUpPressed)
	{
		position = XMVectorAdd(position, XMVectorSet(0.0f, this->_MovementSpeed * deltaTime, 0.0f, 0.0f));
	}
	
	if (this->_StrafeDownPressed)
	{
		position = XMVectorAdd(position, XMVectorSet(0.0f, -this->_MovementSpeed * deltaTime, 0.0f, 0.0f));
	}

	if (this->_StrafeForwardPressed)
	{
		position = XMVectorAdd(position, forwards);
	}

	if (this->_StrafeBackwardPressed)
	{
		position = XMVectorAdd(position, XMVectorNegate(forwards));
	}

	if (this->_StrafeRightPressed)
	{
		position = XMVectorAdd(position, right);
	}

	if (this->_StrafeLeftPressed)
	{
		position = XMVectorAdd(position, XMVectorNegate(right));
	}

	XMStoreFloat3(&this->_Position, position);
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
		this->_Pitch -= dy;
		this->_Yaw += dx;
		return true;
	}
	else if (this->_OrbitActive)
	{
		// TODO: Implement!
		return true;
	}
	else if (this->_PanActive)
	{
		// TODO: Implement!
		return true;
	}
	else if (this->_ZoomActive)
	{
		// TODO: Implement!
		return true;
	}
	else
	{
		return false;
	}
}