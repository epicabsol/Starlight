#include "imgui/imgui_custom.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#define TOOLBAR_WIDTH (32)
#define TOOLBAR_PADDING (8)

namespace ImGuiCustom
{
	ShowcaseResources* _Resources = nullptr;

	void LoadResources(ID3D11Device* device)
	{
		assert(_Resources == nullptr);

		_Resources = new ShowcaseResources(device);
	}

	void UnloadResources()
	{
		assert(_Resources != nullptr);

		delete _Resources;
		_Resources = nullptr;
	}

	bool BeginMenu(const char* label)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		bool open = ImGui::BeginMenu(label);
		ImGui::PopStyleVar(1);

		return open;
	}

	void EndMenu()
	{
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 2.0f);
		ImGui::EndMenu();
	}

	bool BeginSubmenu(const char* label)
	{
		ImVec2 itemSpacing = ImGui::GetStyle().ItemSpacing;
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(itemSpacing.x * 2.0f, itemSpacing.y));
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + itemSpacing.x);
		if (ImGui::GetCursorPosY() <= 0.0f)
		{
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + itemSpacing.y / 2.0f);

			ImGui::GetCurrentWindow()->DC.MenuColumns.TotalWidth += itemSpacing.x;
		}
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y / 2.0f));

		bool open = ImGui::BeginMenu(label);
		ImGui::PopStyleVar(3);

		if (open)
		{
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() - itemSpacing.y / 2.0f);
		}

		return open;
	}

	void EndSubmenu()
	{
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetStyle().ItemSpacing.y / 2.0f);
		ImGuiCustom::EndMenu();
	}

	bool MenuItem(const char* label, const char* shortcut, bool* selected)
	{
		ImVec2 itemSpacing = ImGui::GetStyle().ItemSpacing;
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(itemSpacing.x * 2.0f, itemSpacing.y));
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + itemSpacing.x);
		if (ImGui::GetCursorPosY() <= 0.0f)
		{
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + itemSpacing.y / 2.0f);

			ImGui::GetCurrentWindow()->DC.MenuColumns.TotalWidth += itemSpacing.x;
		}

		bool clicked = ImGui::MenuItem(label, shortcut, selected);
		ImGui::PopStyleVar(1);

		return clicked;
	}

	void BeginToolbar(const char* name, int x, int y)
	{
		ImGui::SetNextWindowPos(ImVec2(x, y));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
		ImGui::Begin(name, nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
	}

	void EndToolbar()
	{
		ImGui::End();
		ImGui::PopStyleVar(2);
	}

	bool ToolbarButton(const char* name, ShowcaseIcon icon, bool active)
	{
		ImVec2 imageSize = ImVec2((TOOLBAR_WIDTH - TOOLBAR_PADDING) * ImGui::GetIO().FontGlobalScale, (TOOLBAR_WIDTH - TOOLBAR_PADDING) * ImGui::GetIO().FontGlobalScale);

		bool clicked;
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(TOOLBAR_PADDING, TOOLBAR_PADDING));
		ImGui::PushStyleColor(ImGuiCol_Button, active ? ImGui::GetStyle().Colors[ImGuiCol_HeaderHovered] : ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, active ? ImGui::GetStyle().Colors[ImGuiCol_HeaderActive] : ImGui::GetStyle().Colors[ImGuiCol_HeaderHovered]);
		clicked = ImGui::ImageButton(name, (ImTextureID)_Resources->GetIconSRV(icon), imageSize);
		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar();

		return clicked && !active;
	}
}
