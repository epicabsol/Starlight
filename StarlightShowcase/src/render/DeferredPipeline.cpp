#include "render/DeferredPipeline.h"

#include "Window.h"

void DeferredPipeline::CreateSizeDependentResources()
{
	this->_Device->CreateRenderTargetView(this->_OutputTexture, nullptr, &this->_OutputRTV);
	
	D3D11_TEXTURE2D_DESC depthTextureDesc =
	{
		this->_Width,
		this->_Height,
		1,
		1,
		DXGI_FORMAT_R24G8_TYPELESS,
		{ 1, 0 },
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE,
		0,
		0,
	};
	this->_Device->CreateTexture2D(&depthTextureDesc, nullptr, &this->_DepthStencilTexture);
	D3D11_DEPTH_STENCIL_VIEW_DESC depthViewDesc =
	{
		DXGI_FORMAT_D24_UNORM_S8_UINT,
		D3D11_DSV_DIMENSION_TEXTURE2D,
		0,
	};
	depthViewDesc.Texture2D.MipSlice = 0;
	this->_Device->CreateDepthStencilView(this->_DepthStencilTexture, &depthViewDesc, &this->_DepthStencilView);
}

void DeferredPipeline::DestroySizeDependentResources()
{
	if (this->_DepthStencilView != nullptr)
	{
		this->_DepthStencilView->Release();
		this->_DepthStencilView = nullptr;
	}
	if (this->_DepthStencilTexture != nullptr)
	{
		this->_DepthStencilTexture->Release();
		this->_DepthStencilTexture = nullptr;
	}
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

	D3D11_DEPTH_STENCIL_DESC depthDesc =
	{
		true,
		D3D11_DEPTH_WRITE_MASK_ALL,
		D3D11_COMPARISON_LESS,
		false,
		D3D11_DEFAULT_STENCIL_READ_MASK,
		D3D11_DEFAULT_STENCIL_WRITE_MASK,
		{ D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS },
		{ D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS },
	};
	this->_Device->CreateDepthStencilState(&depthDesc, &this->_DepthStencilState);
}

DeferredPipeline::~DeferredPipeline()
{
	this->_DepthStencilState->Release();
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
	this->_ImmediateContext->ClearDepthStencilView(this->_DepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	D3D11_VIEWPORT viewport = { 0.0f, 0.0f, (float)this->_Width, (float)this->_Height, 0.0f, 1.0f };
	this->_ImmediateContext->RSSetViewports(1, &viewport);
	this->_ImmediateContext->OMSetRenderTargets(1, &this->_OutputRTV, this->_DepthStencilView);
	this->_ImmediateContext->OMSetDepthStencilState(this->_DepthStencilState, 0);
}