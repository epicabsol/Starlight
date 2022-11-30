#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d11.h>

class Window;

class DeferredPipeline
{
private:
	Window* _Window;
	ID3D11Texture2D* _OutputTexture;
	ID3D11RenderTargetView* _OutputRTV = nullptr;
	size_t _Width;
	size_t _Height;

public:
	DeferredPipeline(Window* window, ID3D11Texture2D* outputTexture, size_t width, size_t height);
	~DeferredPipeline();

	void ReleaseBuffers();
	void RecreateBuffers(ID3D11Texture2D* outputTexture, size_t width, size_t height);

	void Draw();
};