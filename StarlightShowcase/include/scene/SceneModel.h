#pragma once

#include "SceneObject.h"
#include "util/Strings.h"

class Model;

class SceneModel : public SceneObject
{
private:
	const Model* _Model;

public:
	SceneModel();
	~SceneModel();

	const Model* GetModel() const { return this->_Model; }

	void SetModel(const Model* model, const StringView& modelName);

	virtual void OnDrawProperties() override;
};