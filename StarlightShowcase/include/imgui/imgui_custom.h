#pragma once

/**
 * Dear Imgui custom routines
 */

namespace ImGuiCustom
{
	// Adjusted menu appearance (padding instead of margin)

	bool BeginMenu(const char* label);
	void EndMenu();

	bool BeginSubmenu(const char* label);
	void EndSubmenu();

	bool MenuItem(const char* label, const char* shortcut = nullptr, bool* selected = nullptr);
}