#include "Window.h"

#include "render/DeferredPipeline.h"
#include "Widget.h"
#include "render/Technique.h"
#include "render/ConstantBuffer.h"
#include "render/Mesh.h"

#include "WidgetVertexShader.h"
#include "WidgetPixelShader.h"

#include <imgui/backend/imgui_impl_win32.h>
#include <imgui/backend/imgui_impl_dx11.h>

#include "windowsx.h"

using namespace DirectX;

#define WINDOW_CLASS L"StarlightShowcase"

bool Window::_WindowClassRegistered = false;

void Window::SetWrapCursor(bool wrapCursor)
{
	this->_WrapCursor = wrapCursor;
	if (wrapCursor)
	{
		RECT clipRect;
		GetClientRect(this->_Handle, &clipRect);
		ClientToScreen(this->_Handle, (POINT*)&clipRect.left);
		clipRect.right += clipRect.left;
		clipRect.bottom += clipRect.top;
		ClipCursor(&clipRect);
	}
	else
	{
		ClipCursor(nullptr);
	}
}

Window::Window(HINSTANCE application, wchar_t* caption, HWND parent, int x, int y, int width, int height)
	: Window(&HandleCreation<Window>, application, caption, parent, x, y, width, height)
{

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

// Drawing

void XM_CALLCONV Window::DrawWidgetMesh(const Mesh* mesh, const DirectX::XMMATRIX transform, const DirectX::XMVECTOR colorOverride)
{
	WidgetConstants widget;
	XMStoreFloat4x4(&widget.TransformMatrix, XMMatrixTranspose(transform));
	XMStoreFloat4(&widget.ColorOverride, colorOverride);
	this->_WidgetConstantBuffer->Update(this->_ImmediateContext, widget);

	this->_ImmediateContext->IASetInputLayout(this->_WidgetTechnique->GetInputLayout());
	this->_ImmediateContext->IASetPrimitiveTopology(mesh->GetTopology());
	ID3D11Buffer* vertexBuffers[] =
	{
		mesh->GetVertexBuffer()
	};
	UINT strides[] =
	{
		sizeof(WidgetVertex),
	};
	UINT offsets[] =
	{
		0,
	};
	this->_ImmediateContext->IASetVertexBuffers(0, sizeof(vertexBuffers) / sizeof(vertexBuffers[0]), vertexBuffers, strides, offsets);
	this->_ImmediateContext->IASetIndexBuffer(mesh->GetIndexBuffer(), mesh->GetIndexFormat(), 0);
	this->_ImmediateContext->VSSetShader(this->_WidgetTechnique->GetVertexShader(), nullptr, 0);
	ID3D11Buffer* constantBuffers[] =
	{
		this->_FrameConstantBuffer->GetBuffer(),
		this->_WidgetConstantBuffer->GetBuffer(),
	};
	this->_ImmediateContext->VSSetConstantBuffers(0, sizeof(constantBuffers) / sizeof(constantBuffers[0]), constantBuffers);
	this->_ImmediateContext->PSSetShader(this->_WidgetTechnique->GetPixelShader(), nullptr, 0);
	this->_ImmediateContext->DrawIndexed(mesh->GetIndexCount(), 0, 0);
}

void Window::Update(float deltaTime)
{
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGui::GetIO().FontGlobalScale = ImGui_ImplWin32_GetDpiScaleForHwnd(this->_Handle);

	this->_CameraController.Update(deltaTime);
	this->_Camera.SetPosition(this->_CameraController.GetCameraPosition());
	this->_Camera.SetRotation(this->_CameraController.GetCameraRotation());

	this->OnUpdate(deltaTime);

	ImGui::Render();
}

void Window::Render()
{
	ImGui_ImplDX11_NewFrame();

	// Update frame constants
	FrameConstants frame;
	XMStoreFloat4x4(&frame.ViewMatrix, XMMatrixTranspose(this->_Camera.GetViewMatrix()));
	XMStoreFloat4x4(&frame.ProjectionMatrix, XMMatrixTranspose(this->_Camera.GetProjectionMatrix()));
	this->_FrameConstantBuffer->Update(this->_ImmediateContext, frame);

	// Draw scene
	this->_Pipeline->Draw();

	ID3D11RenderTargetView* rtv = this->_Pipeline->GetOutputRTV();
	this->_ImmediateContext->OMSetRenderTargets(1, &rtv, nullptr);
	this->OnDrawWidgets();

	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void Window::Present()
{
	this->_SwapChain->Present(1, 0);
	this->_ImmediateContext->DiscardView(this->_Pipeline->GetOutputRTV());
	// TODO: Also DiscardView() the depth buffer, once that exists
}

void Window::WaitOnSwapChain()
{
	DWORD result = WaitForSingleObjectEx(this->_SwapChainWaitable, 1000, true);
}

Window::Window(LRESULT(*creationHandler)(Window* window), HINSTANCE application, wchar_t* caption, HWND parent, int x, int y, int width, int height)
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

	this->_WindowCreationHandler = creationHandler;

	this->_Handle = CreateWindowEx(0, WINDOW_CLASS, caption, WS_OVERLAPPEDWINDOW, x, y, width, height, parent, NULL, application, this);

	SetWindowLongPtrW(this->_Handle, GWLP_USERDATA, (size_t)(void*)this);

	this->Show();
}

void Window::OnDrawWidgets()
{

}

void Window::OnUpdate(float deltaTime)
{

}

// Window messages

LRESULT Window::OnCreate()
{
	UINT deviceFlags = 0;
#if _DEBUG
	deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	ID3D11DeviceContext* immediateContext = nullptr;
	HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, NULL, deviceFlags, nullptr, 0, D3D11_SDK_VERSION, &this->_Device, nullptr, &immediateContext);
	if (FAILED(hr))
	{
		OutputDebugStringW(L"Failed to create D3D11 device.");
		this->Close();
		return 0;
	}
	immediateContext->QueryInterface(&this->_ImmediateContext);

	IDXGIFactory* factory = nullptr;
	hr = CreateDXGIFactory(IID_PPV_ARGS(&factory));

	RECT clientRect = { };
	GetClientRect(this->_Handle, &clientRect);
	this->_Camera.SetAspectRatio((float)(clientRect.right - clientRect.left) / (clientRect.bottom - clientRect.top));
	this->_CameraController.SetViewThirdPerson(XMVectorZero(), 10.0f, -3.1415f / 4.0f, -3.1415f / 4.0f);

	DXGI_SWAP_CHAIN_DESC scDesc = { { clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, { 60, 1 }, DXGI_FORMAT_B8G8R8A8_UNORM }, { 1, 0 }, DXGI_USAGE_RENDER_TARGET_OUTPUT, 2, this->_Handle, TRUE, DXGI_SWAP_EFFECT_FLIP_DISCARD, DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT };
	IDXGISwapChain* swapChain = nullptr;
	hr = factory->CreateSwapChain(this->_Device, &scDesc, &swapChain);
	swapChain->QueryInterface(&this->_SwapChain);
	swapChain->Release();
	this->_SwapChainWaitable = this->_SwapChain->GetFrameLatencyWaitableObject();
	factory->Release();
	if (FAILED(hr))
	{
		OutputDebugStringW(L"Failed to create swap chain.");
		this->Close();
		return 0;
	}

	hr = this->_SwapChain->GetBuffer(0, IID_PPV_ARGS(&this->_Backbuffer));

	this->_Pipeline = new DeferredPipeline(this->_Device, this->_Backbuffer, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);

	// Load resources
	
	this->_WidgetTechnique = new Technique(this->_Device, WidgetVertexElements, WidgetVertexElementCount,
		Shaders::WidgetVertexShaderBytes, Shaders::WidgetVertexShaderLength,
		Shaders::WidgetPixelShaderBytes, Shaders::WidgetPixelShaderLength);
	XMFLOAT4X4 identity;
	XMStoreFloat4x4(&identity, DirectX::XMMatrixIdentity());
	this->_WidgetConstantBuffer = new ConstantBuffer<WidgetConstants>(this->_Device, WidgetConstants { identity, XMFLOAT4() });
	this->_FrameConstantBuffer = new ConstantBuffer<FrameConstants>(this->_Device, FrameConstants { identity, identity });

	// Set up Dear Imgui
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(this->_Handle);
	ImGui_ImplDX11_Init(this->_Device, this->_ImmediateContext);

	ImGui_ImplWin32_NewFrame();
	ImGui_ImplDX11_NewFrame();
	ImGui::NewFrame();
	ImGui::GetIO().FontGlobalScale = ImGui_ImplWin32_GetDpiScaleForHwnd(this->_Handle);
	ImGui::Render();

	return 0;
}

LRESULT Window::OnDestroy()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	delete this->_FrameConstantBuffer;
	this->_FrameConstantBuffer = nullptr;
	delete this->_WidgetConstantBuffer;
	this->_WidgetConstantBuffer = nullptr;
	delete this->_WidgetTechnique;
	this->_WidgetTechnique = nullptr;

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
		CloseHandle(this->_SwapChainWaitable);
		this->_SwapChainWaitable = NULL;
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

	ValidateRect(this->_Handle, nullptr);
	
	return 0;
}

LRESULT Window::OnResize(int resizeType, unsigned int clientWidth, unsigned int clientHeight)
{
	if (resizeType == SIZE_MINIMIZED)
	{
		this->_Visible = false;
	}
	else if (resizeType != SIZE_MAXSHOW && resizeType != SIZE_MAXHIDE)
	{
		this->_Visible = true;
		if (this->_Backbuffer != nullptr)
		{
			this->_Pipeline->ReleaseBuffers();
			this->_Backbuffer->Release();
			this->_Backbuffer = nullptr;
		}

		this->_SwapChain->ResizeBuffers(0, clientWidth, clientHeight, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);

		this->_SwapChain->GetBuffer(0, IID_PPV_ARGS(&this->_Backbuffer));
		this->_Pipeline->RecreateBuffers(this->_Backbuffer, (size_t)clientWidth, (size_t)clientHeight);

		this->_Camera.SetAspectRatio((float)clientWidth / clientHeight);
	}
	return 0;
}

LRESULT Window::OnKeyDown(int virtualKey)
{
	if (this->_CameraController.KeyDown(virtualKey))
	{
		return 0;
	}

	return 0;
}

LRESULT Window::OnKeyUp(int virtualKey)
{
	if (this->_CameraController.KeyUp(virtualKey))
	{
		return 0;
	}

	return 0;
}

LRESULT Window::OnMouseDown(int button, int x, int y)
{
	if (this->_CameraController.MouseDown(button))
	{
		GetCursorPos(&this->_OriginalCursorPosition);
		this->_LastCursorPosition.x = x;
		this->_LastCursorPosition.y = y;
		this->SetWrapCursor(true);
		ShowCursor(false);
		return 0;
	}

	return 0;
}

LRESULT Window::OnMouseUp(int button, int x, int y)
{
	if (this->_CameraController.MouseUp(button))
	{
		this->SetWrapCursor(false);
		SetCursorPos(this->_OriginalCursorPosition.x, this->_OriginalCursorPosition.y);
		ShowCursor(true);
		return 0;
	}

	return 0;
}

LRESULT Window::OnMouseMove(int x, int y)
{
	if (this->_IsWrappingCursor)
	{
		return 0;
	}

	if (this->_WrapCursor)
	{
		RECT r;
		GetClientRect(this->_Handle, &r);

		if (x == 0 || y == 0 || x == r.right - 1 || y == r.bottom - 1)
		{
			if (x == 0)
			{
				x = r.right - 2;
				this->_LastCursorPosition.x += r.right;
			}
			if (y == 0)
			{
				y = r.bottom - 2;
				this->_LastCursorPosition.y += r.bottom;
			}
			if (x == r.right - 1)
			{
				x = 1;
				this->_LastCursorPosition.x -= r.right;
			}
			if (y == r.bottom - 1)
			{
				y = 1;
				this->_LastCursorPosition.y -= r.bottom;
			}

			this->_IsWrappingCursor = true;
			POINT newPos = { x, y };
			ClientToScreen(this->_Handle, &newPos);
			SetCursorPos(newPos.x, newPos.y);
			this->_IsWrappingCursor = false;
		}
	}

	float relativeX = (float)x - this->_LastCursorPosition.x;
	float relativeY = (float)y - this->_LastCursorPosition.y;
	if (this->_CameraController.MouseMove(relativeX, relativeY))
	{
		
	}

	this->_LastCursorPosition.x = x;
	this->_LastCursorPosition.y = y;
}

// Window procedure

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#include <stdio.h>
LRESULT Window::WindowProc(HWND handle, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT imguiResult = ImGui_ImplWin32_WndProcHandler(handle, uMsg, wParam, lParam);

	if (imguiResult != 0)
	{
		return imguiResult;
	}
	else
	{
		Window* window = (Window*)GetWindowLongPtrW(handle, GWLP_USERDATA);

		switch (uMsg)
		{
		case WM_CREATE:
		{
			CREATESTRUCT* createStruct = (CREATESTRUCT*)lParam;
			window = (Window*)createStruct->lpCreateParams;
			window->_Handle = handle;
			return window->_WindowCreationHandler(window);
		}
		case WM_DESTROY:
			return window->OnDestroy();
		case WM_PAINT:
			return window->OnPaint();
		case WM_SIZE:
			return window->OnResize(wParam, LOWORD(lParam), HIWORD(lParam));
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			return window->OnKeyDown(wParam);
		case WM_KEYUP:
		case WM_SYSKEYUP:
			return window->OnKeyUp(wParam);
		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
		{
			int button = 0;
			if (uMsg == WM_LBUTTONDOWN)
				button = MOUSE_BUTTON_LEFT;
			else if (uMsg == WM_MBUTTONDOWN)
				button = MOUSE_BUTTON_MIDDLE;
			else if (uMsg == WM_RBUTTONDOWN)
				button = MOUSE_BUTTON_RIGHT;
			return window->OnMouseDown(button, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		}
		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
		{
			int button = 0;
			if (uMsg == WM_LBUTTONUP)
				button = MOUSE_BUTTON_LEFT;
			else if (uMsg == WM_MBUTTONUP)
				button = MOUSE_BUTTON_MIDDLE;
			else if (uMsg == WM_RBUTTONUP)
				button = MOUSE_BUTTON_RIGHT;
			return window->OnMouseUp(button, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		}
		case WM_MOUSEMOVE:
			return window->OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		}
		return DefWindowProc(handle, uMsg, wParam, lParam);
	}
}