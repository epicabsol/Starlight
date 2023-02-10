#include "scene/SceneObject.h"

#include "imgui/imgui.h"

void SceneObject::OnSelected()
{
	this->_IsSelected = true;
}

void SceneObject::OnDeselected()
{
	this->_IsSelected = false;
}

void SceneObject::OnDrawProperties()
{
	ImGui::InputFloat3("Position", &this->_Position.x);
	ImGui::LabelText("Rotation", "<%.02f, %.02f, %.02f, %.02f>", this->_Rotation.x, this->_Rotation.y, this->_Rotation.z, this->_Rotation.w);
}