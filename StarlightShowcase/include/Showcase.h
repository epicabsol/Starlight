#pragma once

#include "Window.h"

#include <vector>

class Mesh;
class SceneObject;
class PointLight;
class Tool;
class SelectTool;
class MoveTool;
class RotateTool;

class Showcase : public Window
{
private:
	// NOTE: Leave resources initialized in OnCreate() uninitialized here, due to OnCreate hackery
	Mesh* _GridMesh;

	std::vector<PointLight*> _PointLights;
	SceneObject* _SelectedObject = nullptr;

	std::vector<Tool*> _Tools;
	Tool* _ActiveTool;
	SelectTool* _SelectTool;
	MoveTool* _MoveTool;
	RotateTool* _RotateTool;

	bool _ObjectsPaneVisible = true;
	bool _PropertiesPaneVisible = true;

public:
	Showcase(HINSTANCE hInstance);

	inline SceneObject* GetSelectedObject() const { return this->_SelectedObject; }
	void SetSelectedObject(SceneObject* selectedObject);
	void SetActiveTool(Tool* tool);

protected:
	virtual void OnDrawWidgets() override;
	virtual void OnUpdate(float timeStep) override;

	LRESULT OnCreate();
	virtual LRESULT OnDestroy() override;

	virtual LRESULT OnMouseDown(int button, int x, int y) override;
	virtual LRESULT OnMouseUp(int button, int x, int y) override;
	virtual LRESULT OnMouseMove(int x, int y) override;

	friend class Window;
};