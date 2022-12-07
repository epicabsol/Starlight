#include "DeferredPipeline.h"

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
	float clearColor[] = { 0.1f, 0.6f, 0.9f, 1.0f };
	this->_ImmediateContext->ClearRenderTargetView(this->_OutputRTV, clearColor);
}