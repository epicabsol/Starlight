#pragma once

#include "ShowcaseResources.h"

/**
 * Dear Imgui custom routines
 */

namespace ImGuiCustom
{
	void LoadResources(struct ::ID3D11Device* device);
	void UnloadResources();

	// Adjusted menu appearance (padding instead of margin)

	bool BeginMenu(const char* label);
	void EndMenu();

	bool BeginSubmenu(const char* label);
	void EndSubmenu();

	bool MenuItem(const char* label, const char* shortcut = nullptr, bool* selected = nullptr);

	// Vertical toolbar

	void BeginToolbar(const char* name, int x, int y);
	void EndToolbar();

	bool ToolbarButton(const char* name, ShowcaseIcon icon, bool active = false);
}