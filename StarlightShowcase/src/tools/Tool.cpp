#include "tools/Tool.h"

Tool::Tool(Showcase* showcase, const char* id, ShowcaseIcon icon) : _Showcase(showcase), _ID(id), _Icon(icon)
{

}

Tool::~Tool()
{

}

void Tool::Activate()
{
	this->_IsActive = true;
}

void Tool::Deactivate()
{
	this->_IsActive = false;
}

void Tool::DrawStatus()
{

}

void Tool::DrawWidgets()
{

}

void Tool::OnKeyDown(int virtualKey)
{

}

void Tool::OnKeyUp(int virtualKey)
{

}

void Tool::OnMouseDown(int button, int x, int y)
{

}

void Tool::OnMouseUp(int button, int x, int y)
{

}

void Tool::OnMouseMove(int x, int y)
{

}
