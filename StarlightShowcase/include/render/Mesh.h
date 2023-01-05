#pragma once

struct ID3D11Device;
struct ID3D11Buffer;
enum DXGI_FORMAT;
typedef enum D3D_PRIMITIVE_TOPOLOGY D3D11_PRIMITIVE_TOPOLOGY;

class Mesh
{
private:
	ID3D11Buffer* _VertexBuffer;
	ID3D11Buffer* _IndexBuffer;
	size_t _VertexCount;
	size_t _IndexCount;
	DXGI_FORMAT _IndexFormat;
	D3D11_PRIMITIVE_TOPOLOGY _Topology;

public:
	Mesh(ID3D11Device* device, void* vertexData, size_t vertexSize, size_t vertexCount, void* indexData, size_t indexCount, DXGI_FORMAT indexFormat, D3D11_PRIMITIVE_TOPOLOGY topology);
	~Mesh();

	inline ID3D11Buffer* GetVertexBuffer() const { return this->_VertexBuffer; }
	inline ID3D11Buffer* GetIndexBuffer() const { return this->_IndexBuffer; }
	inline size_t GetVertexCount() const { return this->_VertexCount; }
	inline size_t GetIndexCount() const { return this->_IndexCount; }
	inline DXGI_FORMAT GetIndexFormat() const { return this->_IndexFormat; }
	inline D3D11_PRIMITIVE_TOPOLOGY GetTopology() const { return this->_Topology; }
};