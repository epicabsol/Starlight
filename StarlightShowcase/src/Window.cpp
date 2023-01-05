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

using namespace DirectX;

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
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGui::GetIO().FontGlobalScale = ImGui_ImplWin32_GetDpiScaleForHwnd(this->_Handle);

	ImGui::ShowDemoWindow();

	ImGui::Begin("debug");
	XMFLOAT3 campos;
	XMStoreFloat3(&campos, this->_Camera.GetPosition());
	ImGui::InputFloat3("camera pos", &campos.x);
	this->_Camera.SetPosition(XMLoadFloat3(&campos));
	ImGui::End();

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

	// Draw widgets
	ID3D11RenderTargetView* rtv = this->_Pipeline->GetOutputRTV();
	this->_ImmediateContext->OMSetRenderTargets(1, &rtv, nullptr);
	this->DrawWidgetMesh(this->_GridMesh, XMMatrixIdentity(), XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f));

	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	this->_SwapChain->Present(1, 0);
}

// Drawing

void XM_CALLCONV Window::DrawWidgetMesh(const Mesh* mesh, const DirectX::XMMATRIX transform, const DirectX::XMVECTOR colorOverride)
{
	WidgetConstants widget;
	XMStoreFloat4x4(&widget.TransformMatrix, transform);
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
	this->_Camera.SetAspectRatio((float)(clientRect.right - clientRect.left) / (clientRect.bottom - clientRect.top));
	this->_Camera.SetPosition(XMVectorSet(-10.0f, 2.0f, -10.0f, 1.0f));
	this->_Camera.SetRotation(XMQuaternionRotationAxis(XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f), -1 * 3.14159265f / 4.0f));

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

	this->_Pipeline = new DeferredPipeline(this->_Device, this->_Backbuffer, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);

	// Load resources
	this->_GridMesh = CreateGridWidgetMesh(this->_Device);
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

	delete this->_GridMesh;
	this->_GridMesh = nullptr;
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

		this->_SwapChain->ResizeBuffers(0, clientWidth, clientHeight, DXGI_FORMAT_UNKNOWN, 0);

		this->_SwapChain->GetBuffer(0, IID_PPV_ARGS(&this->_Backbuffer));
		this->_Pipeline->RecreateBuffers(this->_Backbuffer, (size_t)clientWidth, (size_t)clientHeight);

		this->_Camera.SetAspectRatio((float)clientWidth / clientHeight);
	}
	return 0;
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
			return window->OnCreate();
		}
		case WM_DESTROY:
			return window->OnDestroy();
		case WM_PAINT:
			return window->OnPaint();
		case WM_SIZE:
			return window->OnResize(wParam, LOWORD(lParam), HIWORD(lParam));
		}
		return DefWindowProc(handle, uMsg, wParam, lParam);
	}
}