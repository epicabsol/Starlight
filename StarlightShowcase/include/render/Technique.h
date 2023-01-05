#pragma once

struct ID3D11Device;
struct ID3D11InputLayout;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct D3D11_INPUT_ELEMENT_DESC;

class Technique
{
private:
	ID3D11InputLayout* _InputLayout;
	ID3D11VertexShader* _VertexShader;
	ID3D11PixelShader* _PixelShader;

public:
	Technique(ID3D11Device* device, D3D11_INPUT_ELEMENT_DESC* inputElements, size_t inputElementCount,
		const unsigned char* vertexBytecode, size_t vertexBytecodeLength, 
		const unsigned char* pixelBytecode, size_t pixelBytecodeLength);
	~Technique();

	inline ID3D11InputLayout* GetInputLayout() const { return this->_InputLayout; }
	inline ID3D11VertexShader* GetVertexShader() const { return this->_VertexShader; }
	inline ID3D11PixelShader* GetPixelShader() const { return this->_PixelShader; }
};