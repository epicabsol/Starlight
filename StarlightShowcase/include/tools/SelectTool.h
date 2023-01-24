#pragma once

#include "Tool.h"

class SelectTool : public Tool
{
private:

public:
	SelectTool(Showcase* showcase) : Tool(showcase, "tool_select", ShowcaseIcon_ToolSelect) { }
	virtual ~SelectTool() { }

	virtual const char* GetTooltip() const override;

	virtual void DrawStatus() override;

	virtual void OnMouseDown(int button, int x, int y) override;
};