#pragma once

#include "Window.h"

#include <vector>

class Mesh;
class SceneObject;
class PointLight;

class Showcase : public Window
{
private:
	// NOTE: Leave resources initialized in OnCreate() uninitialized here, due to OnCreate hackery
	Mesh* _GridMesh;

	std::vector<PointLight*> _PointLights;
	SceneObject* _SelectedObject = nullptr;

	bool _ObjectsPaneVisible = true;
	bool _PropertiesPaneVisible = true;

public:
	Showcase(HINSTANCE hInstance);

	inline SceneObject* GetSelectedObject() const { return this->_SelectedObject; }
	void SetSelectedObject(SceneObject* selectedObject);

protected:
	virtual void OnDrawWidgets() override;
	virtual void OnUpdate(float timeStep) override;

	LRESULT OnCreate();
	virtual LRESULT OnDestroy() override;

	friend class Window;
};