#include "render/Technique.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d11.h>

Technique::Technique(ID3D11Device* device, D3D11_INPUT_ELEMENT_DESC* inputElements, size_t inputElementCount,
	const unsigned char* vertexBytecode, size_t vertexBytecodeLength,
	const unsigned char* pixelBytecode, size_t pixelBytecodeLength)
{
	HRESULT hr = device->CreateInputLayout(inputElements, (UINT)inputElementCount, vertexBytecode, vertexBytecodeLength, &this->_InputLayout);
	hr = device->CreateVertexShader(vertexBytecode, vertexBytecodeLength, nullptr, &this->_VertexShader);
	hr = device->CreatePixelShader(pixelBytecode, pixelBytecodeLength, nullptr, &this->_PixelShader);
}

Technique::~Technique()
{
	if (this->_PixelShader != nullptr)
	{
		this->_PixelShader->Release();
		this->_PixelShader = nullptr;
	}
	if (this->_VertexShader != nullptr)
	{
		this->_VertexShader->Release();
		this->_VertexShader = nullptr;
	}
	if (this->_InputLayout != nullptr)
	{
		this->_InputLayout->Release();
		this->_InputLayout = nullptr;
	}
}
