#pragma once

#ifndef UNICODE
#define UNICODE
#endif

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d11_1.h>
#include <dxgi1_3.h>
#include "DirectXMath.h"

#include "Camera.h"
#include "CameraController.h"

#define MOUSE_BUTTON_LEFT (0)
#define MOUSE_BUTTON_RIGHT (1)
#define MOUSE_BUTTON_MIDDLE (2)

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
	LRESULT(*_WindowCreationHandler)(Window*);
	bool _Visible = false;
	bool _WrapCursor = false;
	bool _IsWrappingCursor = false;
	POINT _OriginalCursorPosition;
	POINT _LastCursorPosition;

	Camera _Camera = Camera();
	CameraController _CameraController = CameraController();

	ID3D11Device* _Device = nullptr;
	ID3D11DeviceContext1* _ImmediateContext = nullptr;
	IDXGISwapChain2* _SwapChain = nullptr;
	ID3D11Texture2D* _Backbuffer = nullptr;
	HANDLE _SwapChainWaitable;

	DeferredPipeline* _Pipeline = nullptr;

	Technique* _WidgetTechnique = nullptr;
	ConstantBuffer<WidgetConstants>* _WidgetConstantBuffer = nullptr;
	ConstantBuffer<FrameConstants>* _FrameConstantBuffer = nullptr;

	void SetWrapCursor(bool wrapCursor);

public:
	Window(HINSTANCE application, wchar_t* caption, HWND parent = NULL, int x = CW_USEDEFAULT, int y = CW_USEDEFAULT, int width = CW_USEDEFAULT, int height = CW_USEDEFAULT);
	virtual ~Window();

	inline bool IsVisible() const { return this->_Visible; }
	inline ID3D11Device* GetDevice() const { return this->_Device; }
	inline ID3D11DeviceContext* GetImmediateContext() const { return this->_ImmediateContext; }
	inline DeferredPipeline* GetPipeline() const { return this->_Pipeline; }

	inline Camera* GetCamera() { return &this->_Camera; }
	inline CameraController* GetCameraController() { return &this->_CameraController; }

	void Show();
	void Hide();
	void Close();

	void XM_CALLCONV DrawWidgetMesh(const Mesh* mesh, const DirectX::XMMATRIX transform, const DirectX::XMVECTOR colorOverride);

	void Update(float deltaTime);
	void Render();
	void Present();
	void WaitOnSwapChain();

protected:
	// We can't call overridden functions in our constructor, which is where WM_CREATE is called.
	// Hence, all this malarkey.

	// Thunk that calls a member function (OnCreate), and that we can pass as a normal function pointer
	template <typename TWindow>
	static LRESULT HandleCreation(Window* window)
	{
		TWindow* tWindow = (TWindow*)window;
		return tWindow->OnCreate();
	}

	Window(LRESULT(*creationHandler)(Window* window), HINSTANCE application, wchar_t* caption, HWND parent = NULL, int x = CW_USEDEFAULT, int y = CW_USEDEFAULT, int width = CW_USEDEFAULT, int height = CW_USEDEFAULT);
	
protected:

	virtual void OnDrawWidgets();
	virtual void OnUpdate(float deltaTime);

	// OnCreate can't be virtual, as it is called from within the constructor
	/*virtual*/ LRESULT OnCreate();
	virtual LRESULT OnDestroy();
	virtual LRESULT OnPaint();
	virtual LRESULT OnResize(int resizeType, unsigned int clientWidth, unsigned int clientHeight);
	virtual LRESULT OnKeyDown(int virtualKey);
	virtual LRESULT OnKeyUp(int virtualKey);
	virtual LRESULT OnMouseDown(int button, int x, int y);
	virtual LRESULT OnMouseUp(int button, int x, int y);
	virtual LRESULT OnMouseMove(int x, int y);

private:
	static LRESULT CALLBACK WindowProc(HWND handle, UINT uMsg, WPARAM wParam, LPARAM lParam);
};