#pragma once

#include "ShowcaseResources.h"

class Showcase;

class Tool
{
private:
	Showcase* _Showcase;
	const char* _ID;
	ShowcaseIcon _Icon;
	bool _IsActive = false;

protected:
	Tool(Showcase* showcase, const char* id, ShowcaseIcon icon);
	virtual ~Tool();

	inline Showcase* GetShowcase() const { return this->_Showcase; }

public:
	inline const char* GetID() const { return this->_ID; }
	inline ShowcaseIcon GetIcon() const { return this->_Icon; }
	inline bool IsActive() const { return this->_IsActive; }
	virtual const char* GetTooltip() const { return ""; }

	virtual void Activate();
	virtual void Deactivate();
	virtual void DrawStatus();
	virtual void DrawWidgets();

	virtual void OnKeyDown(int virtualKey);
	virtual void OnKeyUp(int virtualKey);
	virtual void OnMouseDown(int button, int x, int y);
	virtual void OnMouseUp(int button, int x, int y);
	virtual void OnMouseMove(int x, int y);
};