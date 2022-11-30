#include "Window.h"

#include "DeferredPipeline.h"

#define WINDOW_CLASS L"StarlightShowcase"

bool Window::_WindowClassRegistered = false;

Window::Window(HINSTANCE application, wchar_t* caption, HWND parent, int x, int y, int width, int height)
{
	// Register the window class if it has not already been registered
	if (!Window::_WindowClassRegistered)
	{
		WNDCLASSW windowClass = { };
		windowClass.lpfnWndProc = &Window::WindowProc;
		windowClass.hInstance = application;
		windowClass.lpszClassName = WINDOW_CLASS;
		windowClass.hCursor = LoadCursorW(NULL, IDC_ARROW);
		RegisterClassW(&windowClass);

		Window::_WindowClassRegistered = true;
	}

	this->_Handle = CreateWindowEx(0, WINDOW_CLASS, caption, WS_OVERLAPPEDWINDOW, x, y, width, height, parent, NULL, application, this);

	SetWindowLongPtrW(this->_Handle, GWLP_USERDATA, (size_t)(void*)this);

	this->Show();
}

Window::~Window()
{
	DestroyWindow(this->_Handle);
	this->_Handle = NULL;
}

void Window::Show()
{
	ShowWindow(this->_Handle, SW_SHOW);
}

void Window::Hide()
{
	ShowWindow(this->_Handle, SW_MINIMIZE);
}

void Window::Close()
{
	CloseWindow(this->_Handle);
}

void Window::Update(float deltaTime)
{

}

void Window::Render()
{
	this->_Pipeline->Draw();
	this->_SwapChain->Present(0, 0);
}

// Window messages

LRESULT Window::OnCreate()
{
	UINT deviceFlags = 0;
#if _DEBUG
	deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, NULL, deviceFlags, nullptr, 0, D3D11_SDK_VERSION, &this->_Device, nullptr, &this->_ImmediateContext);
	if (FAILED(hr))
	{
		OutputDebugStringW(L"Failed to create D3D11 device.");
		this->Close();
		return 0;
	}

	IDXGIFactory* factory = nullptr;
	hr = CreateDXGIFactory(IID_PPV_ARGS(&factory));

	RECT clientRect = { };
	GetClientRect(this->_Handle, &clientRect);

	DXGI_SWAP_CHAIN_DESC scDesc = { { clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, { 60, 1 }, DXGI_FORMAT_B8G8R8A8_UNORM }, { 1, 0 }, DXGI_USAGE_RENDER_TARGET_OUTPUT, 2, this->_Handle, TRUE, DXGI_SWAP_EFFECT_FLIP_DISCARD, 0 };
	hr = factory->CreateSwapChain(this->_Device, &scDesc, &this->_SwapChain);
	factory->Release();
	if (FAILED(hr))
	{
		OutputDebugStringW(L"Failed to create swap chain.");
		this->Close();
		return 0;
	}

	hr = this->_SwapChain->GetBuffer(0, IID_PPV_ARGS(&this->_Backbuffer));

	this->_Pipeline = new DeferredPipeline(this, this->_Backbuffer, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);

	return 0;
}

LRESULT Window::OnDestroy()
{
	if (this->_Pipeline != nullptr)
	{
		delete this->_Pipeline;
		this->_Pipeline = nullptr;
	}
	if (this->_Backbuffer != nullptr)
	{
		this->_Backbuffer->Release();
		this->_Backbuffer = nullptr;
	}
	if (this->_SwapChain != nullptr)
	{
		this->_SwapChain->Release();
		this->_SwapChain = nullptr;
	}
	if (this->_ImmediateContext != nullptr)
	{
		this->_ImmediateContext->Release();
		this->_ImmediateContext = nullptr;
	}
	if (this->_Device != nullptr)
	{
		this->_Device->Release();
		this->_Device = nullptr;
	}

	PostQuitMessage(0);
	return 0;
}

LRESULT Window::OnPaint()
{
	this->Render();
	
	return 0;
}

// Window procedure

LRESULT Window::WindowProc(HWND handle, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	Window* window = (Window*)GetWindowLongPtrW(handle, GWLP_USERDATA);

	switch (uMsg)
	{
	case WM_CREATE:
	{
		CREATESTRUCT* createStruct = (CREATESTRUCT*)lParam;
		window = (Window*)createStruct->lpCreateParams;
		window->_Handle = handle;
		return window->OnCreate();
	}
	case WM_DESTROY:
		return window->OnDestroy();
	case WM_PAINT:
		return window->OnPaint();
	}
	return DefWindowProc(handle, uMsg, wParam, lParam);
}