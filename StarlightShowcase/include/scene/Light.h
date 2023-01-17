#pragma once

#include "SceneObject.h"

class Light : public SceneObject
{
private:
	DirectX::XMFLOAT3 _Color;
	bool _NeedsMaskUpdate;

	// TODO: Store light masks

public:

	inline bool NeedsMaskUpdate() const { return this->_NeedsMaskUpdate; }
	inline void InvalidateMasks() { this->_NeedsMaskUpdate = true; }

	virtual void OnDrawProperties() override;

protected:
	void DrawInvalidatingFloatProperty(const char* label, float* value);
	void DrawInvalidatingColor3Property(const char* label, float* value);
};