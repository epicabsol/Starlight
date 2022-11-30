#pragma once

#ifndef UNICODE
#define UNICODE
#endif

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d11.h>

class DeferredPipeline;

class Window
{
private:
	HWND _Handle;
	static bool _WindowClassRegistered;

	ID3D11Device* _Device = nullptr;
	ID3D11DeviceContext* _ImmediateContext = nullptr;
	IDXGISwapChain* _SwapChain = nullptr;
	ID3D11Texture2D* _Backbuffer = nullptr;

	DeferredPipeline* _Pipeline = nullptr;

public:
	Window(HINSTANCE application, wchar_t* caption, HWND parent = NULL, int x = CW_USEDEFAULT, int y = CW_USEDEFAULT, int width = CW_USEDEFAULT, int height = CW_USEDEFAULT);
	~Window();

	inline ID3D11Device* GetDevice() const { return this->_Device; }
	inline ID3D11DeviceContext* GetImmediateContext() const { return this->_ImmediateContext; }

	void Show();
	void Hide();
	void Close();

	void Update(float deltaTime);
	void Render();

protected:
	LRESULT OnCreate();
	LRESULT OnDestroy();
	LRESULT OnPaint();

private:
	static LRESULT CALLBACK WindowProc(HWND handle, UINT uMsg, WPARAM wParam, LPARAM lParam);
};