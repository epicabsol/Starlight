#pragma once

#include "DirectXMath.h"

#include "util/Strings.h"

/**
 * The base class for transformable, selectable objects.
 */
class SceneObject
{
private:
	InlineString<256> _DisplayName;
	DirectX::XMFLOAT3 _Position = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	DirectX::XMFLOAT4 _Rotation = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

	bool _IsSelected = false;

protected:
	inline SceneObject(const StringView& displayName) : _DisplayName(displayName) { }

public:
	inline const StringView& GetDisplayName() const { return this->_DisplayName; }
	inline DirectX::XMVECTOR XM_CALLCONV GetPosition() const { return DirectX::XMLoadFloat3(&this->_Position); }
	inline DirectX::XMVECTOR XM_CALLCONV GetRotation() const { return DirectX::XMLoadFloat4(&this->_Rotation); }
	inline bool IsSelected() const { return this->_IsSelected; }

	inline void SetDisplayName(const StringView& displayName) { this->_DisplayName = displayName; }
	inline void XM_CALLCONV SetPosition(const DirectX::XMVECTOR position) { DirectX::XMStoreFloat3(&this->_Position, position); }
	inline void XM_CALLCONV SetRotation(const DirectX::XMVECTOR rotation) { DirectX::XMStoreFloat4(&this->_Rotation, rotation); }

	virtual void OnSelected();
	virtual void OnDeselected();
	virtual void OnDrawProperties();
};