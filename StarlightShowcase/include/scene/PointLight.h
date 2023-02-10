#pragma once

#include "Light.h"

class PointLight : public Light
{
private:
	float _Radius = 0.1f;
	float _AttenuationConstant = 1.0f;
	float _AttenuationLinear = 0.22f;
	float _AttenuationQuadratic = 0.20f;

public:
	inline PointLight(const StringView& displayName) : Light(displayName) { }

	inline float GetRadius() const { return this->_Radius; }
	inline float GetAttenuationConstant() const { return this->_AttenuationConstant; }
	inline float GetAttenuationLinear() const { return this->_AttenuationLinear; }
	inline float GetAttenuationQuadratic() const { return this->_AttenuationQuadratic; }

	virtual void OnDrawProperties() override;
};