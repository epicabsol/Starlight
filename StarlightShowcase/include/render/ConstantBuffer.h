#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d11.h>

template <typename TData>
class ConstantBuffer
{
private:
	ID3D11Buffer* _Buffer;

public:
	ConstantBuffer(ID3D11Device* device, const TData& initialValue)
	{
		D3D11_BUFFER_DESC desc = { sizeof(TData), D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE, 0, sizeof(TData) };
		D3D11_SUBRESOURCE_DATA data = { &initialValue, 0, 0 };
		HRESULT hr = device->CreateBuffer(&desc, &data, &this->_Buffer);
	}

	~ConstantBuffer()
	{
		if (this->_Buffer != nullptr)
		{
			this->_Buffer->Release();
			this->_Buffer = nullptr;
		}
	}

	inline ID3D11Buffer* GetBuffer() const { return this->_Buffer; }

	void Update(ID3D11DeviceContext* context, const TData& value)
	{
		D3D11_MAPPED_SUBRESOURCE subresource = { };
		HRESULT hr = context->Map(this->_Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);
		memcpy(subresource.pData, &value, sizeof(TData));
		context->Unmap(this->_Buffer, 0);
	}
};