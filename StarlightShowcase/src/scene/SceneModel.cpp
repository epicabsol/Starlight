#include "scene/SceneModel.h"

#include <string>

#include "Model.h"
#include "render/Mesh.h"
#include "imgui/imgui.h"

SceneModel::SceneModel() : SceneObject("<no model loaded>"), _Model(nullptr)
{

}

SceneModel::~SceneModel()
{

}

void SceneModel::SetModel(const Model* model, const StringView& modelName)
{
	this->_Model = model;
	this->SetDisplayName(modelName);
}

void SceneModel::OnDrawProperties()
{
	SceneObject::OnDrawProperties();

	if (this->_Model != nullptr)
	{
		int vertexCount = 0;
		int triangleCount = 0;
		for (size_t i = 0; i < this->_Model->GetSectionCount(); i++)
		{
			vertexCount += this->_Model->GetSection(i)->GetMesh()->GetVertexCount();
			triangleCount += this->_Model->GetSection(i)->GetMesh()->GetIndexCount() / 3;
		}

		ImGui::Text("%s (%d vertices, %d triangles)", this->GetDisplayName().GetData(), vertexCount, triangleCount);
	}
	else
	{
		ImGui::Text("%s (no model loaded)", this->GetDisplayName().GetData());
	}
}