
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <d3d12.h>
#include <dxgi.h>
#include <stdint.h>

#include "Starlight.hpp"

#include "stb_image.h"
#include "stb_image_write.h"

using namespace DirectX;

#define BYTES_PER_PIXEL (4)
#define LIGHT_MASK_RESOLUTION (1024)
#define INPUT_MESH_FILENAME "test_mesh.obj"
#define OUTPUT_FILENAME "output.png"

struct DemoVertex {
	XMFLOAT3 Position;
	XMFLOAT3 Normal;
	XMFLOAT2 TexCoord;
	XMFLOAT2 LightmapTexCoord;
};

ID3D12Device* _Device = nullptr;
Starlight* _Starlight = nullptr;

DemoVertex _PlaneVertices[] = {
	{ { -1, 0, -1 }, { 0, 1, 0 }, { 0, 0 }, { 0, 0 } },
	{ { 1, 0, -1 }, { 0, 1, 0 }, { 1, 0 }, { 1, 0 } },
	{ { 1, 0, 1 }, { 0, 1, 0 }, { 1, 1 }, { 1, 1 } },
	{ { -1, 0, 1 }, { 0, 1, 0 }, { 0, 1 }, { 0, 1 } },
};
uint8_t _PlaneIndices[] = {
	0, 1, 2,
	0, 2, 3,
};

inline constexpr unsigned int Align(unsigned int size, unsigned int alignment)
{
	return (size + (alignment - 1)) & ~(alignment - 1);
}

void EnableD3D12DebugLayer()
{
	ID3D12Debug* debugController = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
		debugController->Release();
	}
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, wchar_t* pCmdLine, int nCmdShow)
{
	EnableD3D12DebugLayer();

	HRESULT hr = S_OK;

	//
	// INITIALIZE SYSTEM
	//

	// Create the D3D12 device
	hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&_Device));
	if (FAILED(hr))
	{
		OutputDebugStringW(L"Failed to create D3D12 12.2 device.");
		return 1;
	}

	// Initialize Starlight with the d3d12 device
	_Starlight = Starlight::CreateStarlight(_Device);
	if (_Starlight == nullptr)
	{
		OutputDebugStringW(L"Failed to create bake context.");
		return 1;
	}

	//
	// UPLOAD CONTENT
	//

	MeshInfo firstMesh = {
		&_PlaneVertices[0], sizeof(_PlaneVertices[0]), sizeof(_PlaneVertices) / sizeof(_PlaneVertices[0]),
		&_PlaneIndices[0], sizeof(_PlaneIndices[0]), sizeof(_PlaneIndices) / sizeof(_PlaneIndices[0]),
		offsetof(DemoVertex, Position),
		offsetof(DemoVertex, Normal),
		offsetof(DemoVertex, TexCoord),
	};
	XMStoreFloat4x4(&firstMesh.Transform, XMMatrixIdentity());
	uint8_t firstDiffuseTexture[] = {
		220, 220, 220, 255, // Light grey, opaque
		220, 220, 220, 255,
		220, 220, 220, 255,
		220, 220, 220, 255,
	};
	uint8_t firstNormalRoughnessMetallicTexture[] = {
		127, 127, 255, 0, // Flat normal, fully rough, non-metallic
		127, 127, 255, 0,
		127, 127, 255, 0,
		127, 127, 255, 0,
	};
	MaterialInfo firstMaterial = {
		{ &firstDiffuseTexture[0], BYTES_PER_PIXEL * 2, 2, 2 },
		{ &firstNormalRoughnessMetallicTexture[0], BYTES_PER_PIXEL * 2, 2, 2 },
		{ 1.0f, 1.0f, 1.0f },
	};
	_Starlight->AddMesh(&firstMesh, &firstMaterial);

	//
	// DO BAKE
	//

	PointLight pointLight = { { 1, 3, 1 }, 0.5f, 0.5f, 0.01f };
	constexpr size_t stride = Align(BYTES_PER_PIXEL * LIGHT_MASK_RESOLUTION, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
	uint8_t* pointLightMaskBytes = new uint8_t[stride * LIGHT_MASK_RESOLUTION];
	OutputBuffer pointLightMask = { &pointLightMaskBytes[0], stride, LIGHT_MASK_RESOLUTION, LIGHT_MASK_RESOLUTION };
	_Starlight->BakePointLight(&pointLight, &pointLightMask);

	// Save point light's mask's pixel data, which is in `pointLightMaskBytes` - or do whatever else you want with it
	stbi_write_png(OUTPUT_FILENAME, LIGHT_MASK_RESOLUTION, LIGHT_MASK_RESOLUTION, 4, pointLightMaskBytes, (int)stride);

	delete[] pointLightMaskBytes;

	//
	// CLEANUP
	//
	
	delete _Starlight;
	_Starlight = nullptr;
	_Device->Release();
	_Device = nullptr;

	return 0;
}