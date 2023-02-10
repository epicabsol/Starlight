#pragma once

#include "SceneObject.h"

class Light : public SceneObject
{
private:
	DirectX::XMFLOAT3 _Color = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
	bool _NeedsMaskUpdate = true;

	// TODO: Store light masks

public:

	inline bool NeedsMaskUpdate() const { return this->_NeedsMaskUpdate; }
	inline void InvalidateMasks() { this->_NeedsMaskUpdate = true; }

	virtual void OnDrawProperties() override;

protected:
	inline Light(const StringView& displayName) : SceneObject(displayName) { }

	void DrawInvalidatingFloatProperty(const char* label, float* value);
	void DrawInvalidatingColor3Property(const char* label, float* value);
};