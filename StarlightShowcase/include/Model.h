#pragma once

#include <DirectXMath.h>

class Mesh;
struct ID3D11Device;
struct ID3D11ShaderResourceView;
struct D3D11_INPUT_ELEMENT_DESC;

/**
 * A vertex for opaque PBR models
 */
struct ModelVertex
{
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT3 Tangent;
	DirectX::XMFLOAT2 TexCoord;
	DirectX::XMFLOAT2 LightmapTexCoord;
};

extern D3D11_INPUT_ELEMENT_DESC ModelVertexElements[];
extern size_t ModelVertexElementCount;

struct ModelConstants
{
	DirectX::XMFLOAT4X4 TransformMatrix;
};

class ModelSection
{
private:
	const Mesh* _Mesh;
	ID3D11ShaderResourceView* _DiffuseSRV;
	ID3D11ShaderResourceView* _NormalRoughnessMetallicSRV;

public:
	ModelSection(const Mesh* mesh, ID3D11ShaderResourceView* diffuseSRV, ID3D11ShaderResourceView* normalMetallicRoughnessSRV);
	~ModelSection();

	inline const Mesh* GetMesh() const { return this->_Mesh; }
	inline ID3D11ShaderResourceView* GetDiffuseSRV() const { return this->_DiffuseSRV; }
	inline ID3D11ShaderResourceView* GetNormalRoughnessMetallicSRV() const { return this->_NormalRoughnessMetallicSRV; }
};

class Model
{
private:
	const ModelSection** _Sections;
	size_t _SectionCount;

public:
	Model(const ModelSection** sections, size_t sectionCount);
	~Model();

	inline const ModelSection* GetSection(size_t index) const { return this->_Sections[index]; }
	inline size_t GetSectionCount() const { return this->_SectionCount; }
};