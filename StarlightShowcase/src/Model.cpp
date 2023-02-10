#include "Model.h"

#include "render/Mesh.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d11.h>

using namespace DirectX;

D3D11_INPUT_ELEMENT_DESC ModelVertexElements[]
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 2, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};
size_t ModelVertexElementCount = sizeof(ModelVertexElements) / sizeof(ModelVertexElements[0]);

ModelSection::ModelSection(const Mesh* mesh, ID3D11ShaderResourceView* diffuseSRV, ID3D11ShaderResourceView* normalMetallicRoughnessSRV)
{
	this->_Mesh = mesh;
	this->_DiffuseSRV = diffuseSRV;
	this->_NormalRoughnessMetallicSRV = normalMetallicRoughnessSRV;
}

ModelSection::~ModelSection()
{
	delete this->_Mesh;
	this->_Mesh = nullptr;
	this->_DiffuseSRV->Release();
	this->_DiffuseSRV = nullptr;
	this->_NormalRoughnessMetallicSRV->Release();
	this->_NormalRoughnessMetallicSRV = nullptr;
}

Model::Model(const ModelSection** sections, size_t sectionCount) : _Sections(nullptr), _SectionCount(sectionCount)
{
	this->_Sections = new const ModelSection*[sectionCount];
	for (size_t i = 0; i < sectionCount; i++)
	{
		this->_Sections[i] = sections[i];
	}
}

Model::~Model()
{
	for (size_t i = 0; i < this->_SectionCount; i++)
	{
		delete this->_Sections[i];
	}
	delete[] this->_Sections;
	this->_Sections = nullptr;
}