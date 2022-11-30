#include "DeferredPipeline.h"

#include "Window.h"

DeferredPipeline::DeferredPipeline(Window* window, ID3D11Texture2D* outputTexture, size_t width, size_t height) : _Window(window), _OutputTexture(outputTexture), _Width(width), _Height(height)
{
	this->RecreateBuffers(outputTexture, width, height);
}

DeferredPipeline::~DeferredPipeline()
{
	this->ReleaseBuffers();
}

void DeferredPipeline::ReleaseBuffers()
{
	if (this->_OutputRTV != nullptr)
	{
		this->_OutputRTV->Release();
		this->_OutputRTV = nullptr;
	}
}

void DeferredPipeline::RecreateBuffers(ID3D11Texture2D* outputTexture, size_t width, size_t height)
{
	this->_OutputTexture = outputTexture;
	this->_Width = width;
	this->_Height = height;

	this->_Window->GetDevice()->CreateRenderTargetView(this->_OutputTexture, nullptr, &this->_OutputRTV);
}

void DeferredPipeline::Draw()
{
	float clearColor[] = { 0.1f, 0.6f, 0.9f, 1.0f };
	this->_Window->GetImmediateContext()->ClearRenderTargetView(this->_OutputRTV, clearColor);
}