#pragma once

#ifndef UNICODE
#define UNICODE
#endif

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d11.h>
#include "DirectXMath.h"

#include "Camera.h"

class DeferredPipeline;
class Technique;
class Mesh;
template <typename TData>
class ConstantBuffer;
struct WidgetConstants;

struct FrameConstants
{
	DirectX::XMFLOAT4X4 ViewMatrix;
	DirectX::XMFLOAT4X4 ProjectionMatrix;
};

class Window
{
private:
	HWND _Handle;
	static bool _WindowClassRegistered;
	bool _Visible = false;

	Camera _Camera = Camera();

	ID3D11Device* _Device = nullptr;
	ID3D11DeviceContext* _ImmediateContext = nullptr;
	IDXGISwapChain* _SwapChain = nullptr;
	ID3D11Texture2D* _Backbuffer = nullptr;

	DeferredPipeline* _Pipeline = nullptr;

	Technique* _WidgetTechnique = nullptr;
	ConstantBuffer<WidgetConstants>* _WidgetConstantBuffer = nullptr;
	ConstantBuffer<FrameConstants>* _FrameConstantBuffer = nullptr;
	Mesh* _GridMesh = nullptr;

public:
	Window(HINSTANCE application, wchar_t* caption, HWND parent = NULL, int x = CW_USEDEFAULT, int y = CW_USEDEFAULT, int width = CW_USEDEFAULT, int height = CW_USEDEFAULT);
	~Window();

	inline bool IsVisible() const { return this->_Visible; }
	inline ID3D11Device* GetDevice() const { return this->_Device; }
	inline ID3D11DeviceContext* GetImmediateContext() const { return this->_ImmediateContext; }

	void Show();
	void Hide();
	void Close();

	void Update(float deltaTime);
	void Render();

protected:
	void XM_CALLCONV DrawWidgetMesh(const Mesh* mesh, const DirectX::XMMATRIX transform, const DirectX::XMVECTOR colorOverride);

	LRESULT OnCreate();
	LRESULT OnDestroy();
	LRESULT OnPaint();
	LRESULT OnResize(int resizeType, unsigned int clientWidth, unsigned int clientHeight);

private:
	static LRESULT CALLBACK WindowProc(HWND handle, UINT uMsg, WPARAM wParam, LPARAM lParam);
};