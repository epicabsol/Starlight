#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d11.h>

class Window;

class DeferredPipeline
{
private:
	ID3D11Device* _Device;
	ID3D11DeviceContext* _ImmediateContext;
	ID3D11Texture2D* _OutputTexture;
	ID3D11RenderTargetView* _OutputRTV = nullptr;
	ID3D11Texture2D* _DepthStencilTexture = nullptr;
	ID3D11DepthStencilView* _DepthStencilView = nullptr;
	ID3D11DepthStencilState* _DepthStencilState = nullptr;
	size_t _Width;
	size_t _Height;

	void CreateSizeDependentResources();
	void DestroySizeDependentResources();

public:
	DeferredPipeline(ID3D11Device* device, ID3D11Texture2D* outputTexture, size_t width, size_t height);
	~DeferredPipeline();

	inline ID3D11RenderTargetView* GetOutputRTV() const { return this->_OutputRTV; }
	inline ID3D11DepthStencilView* GetDepthStencilView() const { return this->_DepthStencilView; }
	inline size_t GetWidth() const { return this->_Width; }
	inline size_t GetHeight() const { return this->_Height; }

	void ReleaseBuffers();
	void RecreateBuffers(ID3D11Texture2D* outputTexture, size_t width, size_t height);

	void Draw();
};