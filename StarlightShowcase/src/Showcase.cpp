#include "Showcase.h"

#include "Widget.h"
#include "render/DeferredPipeline.h"
#include "scene/SceneObject.h"
#include "scene/PointLight.h"
#include "scene/SceneModel.h"
#include "imgui/imgui.h"
#include "imgui/imgui_custom.h"

#include "tools/SelectTool.h"
#include "tools/MoveTool.h"
#include "tools/RotateTool.h"

#include "ShowcaseResources.h"

using namespace DirectX;

void Showcase::DrawSceneObjectTreeItem(SceneObject* sceneObject)
{
	ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_Leaf;
	if (sceneObject->IsSelected())
	{
		nodeFlags |= ImGuiTreeNodeFlags_Selected;
	}
	bool clicked = ImGui::TreeNodeEx(sceneObject, nodeFlags, "%s", sceneObject->GetDisplayName().GetData());
	if (ImGui::IsItemClicked())
	{
		this->SetSelectedObject(sceneObject);
	}
	if (clicked)
	{
		ImGui::TreePop();
	}

}

Showcase::Showcase(HINSTANCE hInstance) : Window(&Window::HandleCreation<Showcase>, hInstance, L"Starlight Showcase"), _PointLights(), _ActiveTool(nullptr)
{
	this->_SelectTool = new SelectTool(this);
	this->_Tools.push_back(this->_SelectTool);
	this->SetActiveTool(this->_SelectTool);
	this->_MoveTool = new MoveTool(this);
	this->_Tools.push_back(this->_MoveTool);
	this->_RotateTool = new RotateTool(this);
	this->_Tools.push_back(this->_RotateTool);
	
	this->_PointLights.push_back(new PointLight("Point Light 0"));
	this->_SceneModel = new SceneModel();
	this->SetSelectedObject(this->_PointLights[0]);
}

void Showcase::SetSelectedObject(SceneObject* selectedObject)
{
	if (selectedObject != this->_SelectedObject)
	{
		if (this->_SelectedObject != nullptr)
		{
			this->_SelectedObject->OnDeselected();
		}

		this->_SelectedObject = selectedObject;

		if (this->_SelectedObject != nullptr)
		{
			this->_SelectedObject->OnSelected();
		}
	}
}

void Showcase::SetActiveTool(Tool* tool)
{
	if (tool != this->_ActiveTool)
	{
		if (this->_ActiveTool != nullptr)
		{
			this->_ActiveTool->Deactivate();
		}

		this->_ActiveTool = tool;

		if (tool != nullptr)
		{
			tool->Activate();
		}
	}
}

void Showcase::LoadCornellBoxModel()
{

	this->_SceneModel->SetModel(nullptr, "Cornell Box");
	this->SetSelectedObject(this->_SceneModel);
}

void Showcase::LoadCustomModel()
{
	// TODO: Implement!
	MessageBoxA(0, "Not yet implemented.", "Starlight Showcase", MB_OK);
	//this->SetSelectedObject(this->_SceneModel);
}

void Showcase::OnDrawWidgets()
{
	this->DrawWidgetMesh(this->_GridMesh, DirectX::XMMatrixIdentity(), DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f));

	// Draw tool widgets through scene and grid

	this->GetImmediateContext()->ClearDepthStencilView(this->GetPipeline()->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	if (this->_ActiveTool != nullptr)
	{
		this->_ActiveTool->DrawWidgets();
	}
}

void Showcase::OnUpdate(float timeStep)
{
	Window::OnUpdate(timeStep);

#ifdef _DEBUG
	static bool showImGuiDemo = false;
#endif

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6)); // Add some extra vertical padding
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGuiCustom::BeginMenu("Model"))
		{
			if (ImGuiCustom::BeginSubmenu("Sample Models"))
			{
				if (ImGuiCustom::MenuItem("Use Cornell Box"))
				{
					this->LoadCornellBoxModel();
				}

				ImGuiCustom::EndSubmenu();
			}

			if (ImGuiCustom::MenuItem("Use Custom Model..."))
			{
				this->LoadCustomModel();
			}

			ImGuiCustom::EndMenu();
		}

		if (ImGuiCustom::BeginMenu("Window"))
		{
			ImGuiCustom::MenuItem("Show Objects Pane", nullptr, &this->_ObjectsPaneVisible);
			ImGuiCustom::MenuItem("Show Properties Pane", nullptr, &this->_PropertiesPaneVisible);

#ifdef _DEBUG
			ImGui::Separator();
			ImGuiCustom::MenuItem("Show ImGui Demo Window", nullptr, &showImGuiDemo);
#endif

			ImGuiCustom::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}
#ifdef _DEBUG
	if (showImGuiDemo)
	{
		ImGui::ShowDemoWindow();
	}
#endif
	ImGui::PopStyleVar(1);

	// Status bar
	if (ImGuiCustom::BeginStatusBar())
	{
		if (this->_ActiveTool != nullptr)
		{
			this->_ActiveTool->DrawStatus();
		}

		ImGuiCustom::EndStatusBar();
	}

	// Properties
	float propertiesWidth = 300.0f * ImGui::GetIO().FontGlobalScale;
	float workspaceHeight = ImGui::GetMainViewport()->WorkSize.y;
	if (this->_PropertiesPaneVisible)
	{
		ImGui::SetNextWindowSize(ImVec2(propertiesWidth, workspaceHeight + 2.0f));
		ImGui::SetNextWindowPos(ImVec2(ImGui::GetMainViewport()->WorkPos.x + ImGui::GetMainViewport()->WorkSize.x - propertiesWidth, ImGui::GetMainViewport()->WorkPos.y - 1.0f));
		if (ImGui::Begin("Properties", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse))
		{
			if (this->_SelectedObject != nullptr)
			{
				this->_SelectedObject->OnDrawProperties();
			}
			else
			{
				float available = ImGui::GetContentRegionMax().x;
				ImVec2 textSize = ImGui::CalcTextSize("No selection");
				ImGui::SetCursorPosX(available / 2.0f - textSize.x / 2.0f);
				ImGui::Text("No selection");
			}
		}
		ImGui::End();
	}

	// Objects
	if (this->_ObjectsPaneVisible)
	{
		ImGui::SetNextWindowSize(ImVec2(propertiesWidth, workspaceHeight + 2.0f));
		ImGui::SetNextWindowPos(ImVec2(ImGui::GetMainViewport()->WorkPos.x, ImGui::GetMainViewport()->WorkPos.y - 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 4.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
		if (ImGui::Begin("Objects", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse))
		{
			const ImGuiTreeNodeFlags defaultNodeFlags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_FramePadding;
			this->DrawSceneObjectTreeItem(this->_SceneModel);

			if (ImGui::TreeNodeEx("Sky Light", defaultNodeFlags | ImGuiTreeNodeFlags_Leaf))
			{
				ImGui::TreePop();
			}

			if (ImGui::TreeNodeEx("Point Lights", defaultNodeFlags | ImGuiTreeNodeFlags_DefaultOpen))
			{
				for (size_t i = 0; i < this->_PointLights.size(); i++)
				{
					this->DrawSceneObjectTreeItem(this->_PointLights[i]);
				}

				ImGui::TreePop();
			}
		}
		ImGui::End();
		ImGui::PopStyleVar(2);

	}

	// Toolbar
	const float toolbarMargin = 4.0f;
	ImGuiCustom::BeginToolbar("Toolbar", (this->_ObjectsPaneVisible ? propertiesWidth : 0.0f) + toolbarMargin, ImGui::GetMainViewport()->WorkPos.y + toolbarMargin);

	for (int i = 0; i < this->_Tools.size(); i++)
	{
		Tool* tool = this->_Tools[i];
		if (ImGuiCustom::ToolbarButton(tool->GetID(), tool->GetIcon(), tool == this->_ActiveTool))
		{
			this->_ActiveTool = tool;
		}
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		{
			ImGuiCustom::SetTooltip(tool->GetTooltip());
		}
	}

	ImGuiCustom::EndToolbar();
}

LRESULT Showcase::OnCreate()
{
	LRESULT result = Window::OnCreate();

	ImGuiCustom::LoadResources(this->GetDevice());

	this->_GridMesh = CreateGridWidgetMesh(this->GetDevice());

	return result;
}

LRESULT Showcase::OnDestroy()
{
	this->_Tools.clear();
	delete this->_SelectTool;
	this->_SelectTool = nullptr;
	delete this->_MoveTool;
	this->_MoveTool = nullptr;
	delete this->_RotateTool;
	this->_RotateTool = nullptr;

	delete this->_GridMesh;
	this->_GridMesh = nullptr;

	ImGuiCustom::UnloadResources();

	return Window::OnDestroy();
}

LRESULT Showcase::OnMouseDown(int button, int x, int y)
{
	LRESULT result = Window::OnMouseDown(button, x, y);
	if (result != 0)
	{
		this->_ActiveTool->OnMouseDown(button, x, y);
		result = 0;
	}
	return result;
}

LRESULT Showcase::OnMouseUp(int button, int x, int y)
{
	LRESULT result = Window::OnMouseUp(button, x, y);
	if (result != 0)
	{
		this->_ActiveTool->OnMouseUp(button, x, y);
		result = 0;
	}
	return result;
}

LRESULT Showcase::OnMouseMove(int x, int y)
{
	LRESULT result = Window::OnMouseMove(x, y);
	if (result != 0)
	{
		this->_ActiveTool->OnMouseMove(x, y);
		result = 0;
	}
	return result;
}