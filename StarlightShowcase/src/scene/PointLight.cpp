#include "scene/PointLight.h"

#include "imgui/imgui.h"

void PointLight::OnDrawProperties()
{
	Light::OnDrawProperties();

	this->DrawInvalidatingFloatProperty("Radius", &this->_Radius);

	ImGui::Text("Attenuation:");
	this->DrawInvalidatingFloatProperty("Constant", &this->_AttenuationConstant);
	this->DrawInvalidatingFloatProperty("Linear", &this->_AttenuationLinear);
	this->DrawInvalidatingFloatProperty("Quadratic", &this->_AttenuationQuadratic);
}