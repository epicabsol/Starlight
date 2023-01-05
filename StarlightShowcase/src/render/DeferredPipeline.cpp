#include "render/DeferredPipeline.h"

#include "Window.h"

void DeferredPipeline::CreateSizeDependentResources()
{
	this->_Device->CreateRenderTargetView(this->_OutputTexture, nullptr, &this->_OutputRTV);
}

void DeferredPipeline::DestroySizeDependentResources()
{
	if (this->_OutputRTV != nullptr)
	{
		this->_OutputRTV->Release();
		this->_OutputRTV = nullptr;
	}
}

DeferredPipeline::DeferredPipeline(ID3D11Device* device, ID3D11Texture2D* outputTexture, size_t width, size_t height) : _Device(device), _OutputTexture(outputTexture), _Width(width), _Height(height)
{
	device->GetImmediateContext(&this->_ImmediateContext);
	this->RecreateBuffers(outputTexture, width, height);
}

DeferredPipeline::~DeferredPipeline()
{
	this->ReleaseBuffers();
	this->_ImmediateContext->Release();
}

void DeferredPipeline::ReleaseBuffers()
{
	this->DestroySizeDependentResources();
}

void DeferredPipeline::RecreateBuffers(ID3D11Texture2D* outputTexture, size_t width, size_t height)
{
	this->_OutputTexture = outputTexture;
	this->_Width = width;
	this->_Height = height;

	this->CreateSizeDependentResources();
}

void DeferredPipeline::Draw()
{
	float clearColor[] = { 0.1f, 0.1f, 0.1f, 1.0f };
	this->_ImmediateContext->ClearRenderTargetView(this->_OutputRTV, clearColor);
	D3D11_VIEWPORT viewport = { 0.0f, 0.0f, (float)this->_Width, (float)this->_Height, 0.0f, 1.0f };
	this->_ImmediateContext->RSSetViewports(1, &viewport);

}