
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <d3d12.h>
#include <dxgi.h>
#include <dxgidebug.h>
#include <DXProgrammableCapture.h>
#include <stdint.h>

#include "Starlight.hpp"

#include "pix3.h"
#include "stb_image.h"
#include "stb_image_write.h"
#include "tiny_obj_loader.hpp"

using namespace DirectX;

#define BYTES_PER_PIXEL (4)
#define LIGHT_MASK_RESOLUTION (1024)
#define INPUT_MESH_FILENAME "test_mesh.obj"
#define OUTPUT_FILENAME "output.png"

struct DemoVertex {
	XMFLOAT3 Position;
	XMFLOAT3 Normal;
	XMFLOAT2 TexCoord;
	XMFLOAT2 LightmapCoord;
};

ID3D12Device* _Device = nullptr;
Starlight* _Starlight = nullptr;

IDXGIDebug* _DXGIDebug = nullptr;
IDXGraphicsAnalysis* _CaptureController = nullptr;

DemoVertex _PlaneVertices[] = {
	{ { -1, 0, -1 }, { 0, 1, 0 }, { 0, 0 }, { 0, 0 } },
	{ { 1, 0, -1 }, { 0, 1, 0 }, { 1, 0 }, { 1, 0 } },
	{ { 1, 0, 1 }, { 0, 1, 0 }, { 1, 1 }, { 1, 1 } },
	{ { -1, 0, 1 }, { 0, 1, 0 }, { 0, 1 }, { 0, 1 } },
};
uint16_t _PlaneIndices[] = {
	0, 1, 2,
	0, 2, 3,
};

inline constexpr unsigned int Align(unsigned int size, unsigned int alignment)
{
	return (size + (alignment - 1)) & ~(alignment - 1);
}

typedef HRESULT(*DXGIGetDebugInterface_t)(const IID& riid, void** ppDebug);

void EnableD3D12DebugLayer()
{
#ifdef _DEBUG
	PIXLoadLatestWinPixGpuCapturerLibrary();
	ID3D12Debug* debugController = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
		debugController->Release();
	}

	HMODULE dxgiDebug = GetModuleHandleW(L"Dxgidebug.dll");
	if (dxgiDebug != nullptr)
	{
		DXGIGetDebugInterface_t dxgiGetDebugInterface = (DXGIGetDebugInterface_t)GetProcAddress(dxgiDebug, "DXGIGetDebugInterface");
		if (dxgiGetDebugInterface != nullptr)
		{
			if (SUCCEEDED(dxgiGetDebugInterface(IID_PPV_ARGS(&_DXGIDebug))))
			{
			}

			if (SUCCEEDED(dxgiGetDebugInterface(IID_PPV_ARGS(&_CaptureController))))
			{
				_CaptureController->BeginCapture();
			}
		}
	}

#endif
}

void ReportDXGILeaks()
{

	if (_DXGIDebug != nullptr)
	{
		_DXGIDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		_DXGIDebug->Release();
		_DXGIDebug = nullptr;
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
	MeshFormat meshFormat = {
		sizeof(DemoVertex),
		sizeof(_PlaneIndices[0]),
		offsetof(DemoVertex, Position),
		offsetof(DemoVertex, Normal),
		offsetof(DemoVertex, TexCoord),
		offsetof(DemoVertex, LightmapCoord),
	};
	_Starlight = Starlight::CreateStarlight(_Device, meshFormat);
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
	//_Starlight->AddMesh(&firstMesh, &firstMaterial);

	// Load the mesh in suzanne.obj
	tinyobj::ObjReaderConfig readerConfig;
	tinyobj::ObjReader reader;
	if (reader.ParseFromFile("assets/suzanne.obj"))
	{
		if (!reader.Warning().empty())
		{
			OutputDebugStringW(L"[WARNING]: OBJ message: ");
			OutputDebugStringA(reader.Warning().c_str());
		}

		for (size_t i = 0; i < reader.GetShapes().size(); i++)
		{
			size_t triangleCount = reader.GetShapes()[i].mesh.num_face_vertices.size();

			// Load the OBJ data into DemoVertexes and corresponding indices
			// We aren't going to try merging duplicate vertices for this demo
			// We also assume that the mesh is triangulated
			DemoVertex* vertices = new DemoVertex[triangleCount * 3];
			uint16_t* indices = new uint16_t[triangleCount * 3];

			for (size_t j = 0; j < triangleCount; j++)
			{
				for (size_t k = 0; k < 3; k++)
				{
					size_t index = j * 3 + k;
					tinyobj::index_t vertexIndices = reader.GetShapes()[i].mesh.indices[index];

					vertices[index].Position.x = reader.GetAttrib().vertices[3 * vertexIndices.vertex_index + 0];
					vertices[index].Position.y = reader.GetAttrib().vertices[3 * vertexIndices.vertex_index + 1];
					vertices[index].Position.z = reader.GetAttrib().vertices[3 * vertexIndices.vertex_index + 2];

					if (vertexIndices.normal_index >= 0)
					{
						vertices[index].Normal.x = reader.GetAttrib().normals[3 * vertexIndices.normal_index + 0];
						vertices[index].Normal.y = reader.GetAttrib().normals[3 * vertexIndices.normal_index + 1];
						vertices[index].Normal.z = reader.GetAttrib().normals[3 * vertexIndices.normal_index + 2];
					}

					if (vertexIndices.texcoord_index >= 0)
					{
						vertices[index].TexCoord.x = reader.GetAttrib().texcoords[2 * vertexIndices.texcoord_index + 0];
						vertices[index].TexCoord.y = reader.GetAttrib().texcoords[2 * vertexIndices.texcoord_index + 1];

						// For OBJ files, just use the texcoord as the lightmap texcoord
						vertices[index].LightmapCoord = vertices[index].TexCoord;
					}

					indices[index] = index;
				}
			}

			MeshInfo meshInfo = { vertices, sizeof(vertices[0]), triangleCount * 3, indices, sizeof(indices[0]), triangleCount * 3};
			_Starlight->AddMesh(&meshInfo, &firstMaterial);

			delete[] indices;
			delete[] vertices;
		}
	}
	else
	{
		printf("[WARNING]: Could not load assets/suzanne.obj!");
	}

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

	if (_CaptureController != nullptr)
	{
		_CaptureController->EndCapture();
		_CaptureController->Release();
		_CaptureController = nullptr;
	}

	ReportDXGILeaks();

	return 0;
}