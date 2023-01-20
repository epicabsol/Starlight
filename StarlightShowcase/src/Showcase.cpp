#include "Showcase.h"

#include "Widget.h"
#include "scene/SceneObject.h"
#include "scene/PointLight.h"
#include "imgui/imgui.h"
#include "imgui/imgui_custom.h"

#include "ShowcaseResources.h"

using namespace DirectX;

Showcase::Showcase(HINSTANCE hInstance) : Window(&Window::HandleCreation<Showcase>, hInstance, L"Starlight Showcase")
{
	this->_PointLights.push_back(new PointLight());
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

void Showcase::OnDrawWidgets()
{
	this->DrawWidgetMesh(this->_GridMesh, DirectX::XMMatrixIdentity(), DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f));
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
					// TODO: Implement!
				}

				ImGuiCustom::EndSubmenu();
			}

			if (ImGuiCustom::MenuItem("Use Custom Model..."))
			{
				// TODO: Implement!
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
			ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Leaf;
			if (ImGui::TreeNodeEx("Model", defaultNodeFlags | ImGuiTreeNodeFlags_Leaf))
			{
				ImGui::TreePop();
			}

			if (ImGui::TreeNodeEx("Sky Light", defaultNodeFlags | ImGuiTreeNodeFlags_Leaf))
			{
				ImGui::TreePop();
			}

			if (ImGui::TreeNodeEx("Point Lights", defaultNodeFlags | ImGuiTreeNodeFlags_DefaultOpen))
			{
				for (size_t i = 0; i < this->_PointLights.size(); i++)
				{
					nodeFlags = defaultNodeFlags | ImGuiTreeNodeFlags_Leaf;
					if (this->_PointLights[i]->IsSelected())
					{
						nodeFlags |= ImGuiTreeNodeFlags_Selected;
					}
					bool clicked = ImGui::TreeNodeEx(this->_PointLights[i], nodeFlags, "Point Light %d", (int)i);
					if (ImGui::IsItemClicked())
					{
						this->SetSelectedObject(this->_PointLights[i]);
					}
					if (clicked)
					{
						ImGui::TreePop();
					}
				}

				ImGui::TreePop();
			}
		}
		ImGui::End();
		ImGui::PopStyleVar(2);

		// Toolbar
		const float toolbarMargin = 4.0f;
		ImGuiCustom::BeginToolbar("Toolbar", propertiesWidth + toolbarMargin, ImGui::GetMainViewport()->WorkPos.y + toolbarMargin);

		static int toolIndex = 0;
		static ShowcaseIcon toolIcons[] =
		{
			ShowcaseIcon_ToolSelect,
			ShowcaseIcon_ToolMove,
			ShowcaseIcon_ToolRotate,
		};
		for (int i = 0; i < 3; i++)
		{
			bool selected = i == toolIndex;
			static char id[10];
			sprintf_s(id, "tool_%d", i);
			if (ImGuiCustom::ToolbarButton(id, toolIcons[i], selected) && i != toolIndex)
			{
				toolIndex = i;
			}
		}

		ImGuiCustom::EndToolbar();
	}
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
	delete this->_GridMesh;
	this->_GridMesh = nullptr;

	ImGuiCustom::UnloadResources();

	return Window::OnDestroy();
}