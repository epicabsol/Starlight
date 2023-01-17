#include "scene/Light.h"

#include "imgui/imgui.h"

void Light::OnDrawProperties()
{
	SceneObject::OnDrawProperties();
	
	this->DrawInvalidatingColor3Property("Light Color", &this->_Color.x);
}

void Light::DrawInvalidatingFloatProperty(const char* label, float* value)
{
	float editableValue = *value;
	ImGui::InputFloat(label, &editableValue);
	if (editableValue != *value)
	{
		*value = editableValue;
		this->InvalidateMasks();
	}
}

void Light::DrawInvalidatingColor3Property(const char* label, float* value)
{
	float editableValue[] = { value[0], value[1], value[2] };
	ImGui::ColorEdit3(label, &editableValue[0], ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
	if (editableValue[0] != value[0] || editableValue[1] != value[1] || editableValue[2] != value[2])
	{
		value[0] = editableValue[0];
		value[1] = editableValue[1];
		value[2] = editableValue[2];
		this->InvalidateMasks();
	}

}