#pragma once

#include "Light.h"

class PointLight : public Light
{
private:
	float _Radius;
	float _AttenuationConstant;
	float _AttenuationLinear;
	float _AttenuationQuadratic;

public:

	inline float GetRadius() const { return this->_Radius; }
	inline float GetAttenuationConstant() const { return this->_AttenuationConstant; }
	inline float GetAttenuationLinear() const { return this->_AttenuationLinear; }
	inline float GetAttenuationQuadratic() const { return this->_AttenuationQuadratic; }

	virtual void OnDrawProperties() override;
};