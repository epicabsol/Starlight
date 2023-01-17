#include "scene/PointLight.h"

#include "imgui/imgui.h"

void PointLight::OnDrawProperties()
{
	Light::OnDrawProperties();

	this->DrawInvalidatingFloatProperty("Radius", &this->_Radius);
	this->DrawInvalidatingFloatProperty("Attenuation: Constant", &this->_AttenuationConstant);
	this->DrawInvalidatingFloatProperty("Attenuation: Linear", &this->_AttenuationLinear);
	this->DrawInvalidatingFloatProperty("Attenuation: Quadratic", &this->_AttenuationQuadratic);
}