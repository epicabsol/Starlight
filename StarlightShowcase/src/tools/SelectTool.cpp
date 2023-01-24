#include "tools/SelectTool.h"

#include "imgui/imgui.h"

const char* SelectTool::GetTooltip() const
{
	return "Select Tool";
}

void SelectTool::DrawStatus()
{
	ImGui::Text("Left-click on an object to select it.");
}

void SelectTool::OnMouseDown(int button, int x, int y)
{
	// TODO: Implement!
}