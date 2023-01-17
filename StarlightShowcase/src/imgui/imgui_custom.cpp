#include "imgui/imgui_custom.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace ImGuiCustom
{
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
}
