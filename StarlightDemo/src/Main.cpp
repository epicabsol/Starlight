
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
#define SKYBOX_FACE_COUNT (6)
#define LIGHT_MASK_RESOLUTION (1024)
#define INPUT_MESH_FILENAME "test_mesh.obj"

#define BAKE_POINT_LIGHT
#define BAKE_DIRECTIONAL_LIGHT
#define BAKE_SKY_LIGHT

struct DemoVertex {
	XMFLOAT3 Position;
	XMFLOAT3 Normal;
	XMFLOAT3 Tangent;
	XMFLOAT2 TexCoord;
	XMFLOAT2 LightmapCoord;
};

ID3D12Device* _Device = nullptr;
Starlight* _Starlight = nullptr;

IDXGIDebug* _DXGIDebug = nullptr;
IDXGraphicsAnalysis* _CaptureController = nullptr;

DemoVertex _PlaneVertices[] = {
	{ { -1, 0, -1 }, { 0, 1, 0 }, { 1, 0, 0 }, { 0, 0 }, { 0, 0 } },
	{ { 1, 0, -1 }, { 0, 1, 0 }, { 1, 0, 0 }, { 1, 0 }, { 1, 0 } },
	{ { 1, 0, 1 }, { 0, 1, 0 }, { 1, 0, 0 }, { 1, 1 }, { 1, 1 } },
	{ { -1, 0, 1 }, { 0, 1, 0 }, { 1, 0, 0 }, { 0, 1 }, { 0, 1 } },
};
uint32_t _PlaneIndices[] = {
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

		// Enable GPU-based validation
		ID3D12Debug1* debugController1 = nullptr;
		if (SUCCEEDED(debugController->QueryInterface(&debugController1)))
		{
			debugController1->SetEnableGPUBasedValidation(true);
			debugController1->Release();
		}

		debugController->Release();
	}

	HMODULE dxgiDebug = LoadLibraryW(L"Dxgidebug.dll");
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

		// TODO: Free dxgidebug.dll
	}
}

// Based on https://marti.works/posts/post-calculating-tangents-for-your-mesh/post/
void ComputeTangents(DemoVertex* vertices, size_t vertexCount, uint32_t* indices, size_t indexCount)
{
	XMFLOAT3* tanA = new XMFLOAT3[vertexCount];
	XMFLOAT3* tanB = new XMFLOAT3[vertexCount];

	for (size_t i = 0; i < vertexCount; i++)
	{
		tanA[i] = XMFLOAT3(0.0f, 0.0f, 0.0f);
		tanB[i] = XMFLOAT3(0.0f, 0.0f, 0.0f);
	}

	for (size_t i = 0; i < indexCount; i += 3)
	{
		uint32_t i0 = indices[i];
		uint32_t i1 = indices[i + 1];
		uint32_t i2 = indices[i + 2];

		XMVECTOR pos0 = XMLoadFloat3(&vertices[i0].Position);
		XMVECTOR pos1 = XMLoadFloat3(&vertices[i1].Position);
		XMVECTOR pos2 = XMLoadFloat3(&vertices[i2].Position);

		XMVECTOR tex0 = XMLoadFloat2(&vertices[i0].TexCoord);
		XMVECTOR tex1 = XMLoadFloat2(&vertices[i1].TexCoord);
		XMVECTOR tex2 = XMLoadFloat2(&vertices[i2].TexCoord);

		XMVECTOR edge1 = XMVectorSubtract(pos1, pos0);
		XMVECTOR edge2 = XMVectorSubtract(pos2, pos0);

		XMVECTOR uv1 = XMVectorSubtract(tex1, tex0);
		XMVECTOR uv2 = XMVectorSubtract(tex2, tex0);

		XMFLOAT2 uv1floats;
		XMFLOAT2 uv2floats;
		XMStoreFloat2(&uv1floats, uv1);
		XMStoreFloat2(&uv2floats, uv2);
		float r = 1.0f / (uv1floats.x * uv2floats.y - uv1floats.y * uv2floats.x);

		XMVECTOR tangent = XMVectorScale(XMVectorSubtract(XMVectorScale(edge1, uv2floats.y), XMVectorScale(edge2, uv1floats.y)), r);
		XMVECTOR bitangent = XMVectorScale(XMVectorSubtract(XMVectorScale(edge1, uv2floats.x), XMVectorScale(edge2, uv1floats.x)), r);

		for (size_t j = 0; j < 3; j++)
		{
			XMStoreFloat3(&tanA[i0 + j], XMVectorAdd(tangent, XMLoadFloat3(&tanA[i0 + j])));
		}
		for (size_t j = 0; j < 3; j++)
		{
			XMStoreFloat3(&tanB[i0 + j], XMVectorAdd(bitangent, XMLoadFloat3(&tanB[i0 + j])));
		}
	}

	for (size_t i = 0; i < vertexCount; i++)
	{
		XMVECTOR n = XMLoadFloat3(&vertices[i].Normal);
		XMVECTOR t0 = XMLoadFloat3(&tanA[i]);
		XMVECTOR t1 = XMLoadFloat3(&tanB[i]);

		XMVECTOR t = XMVector3Normalize(XMVectorSubtract(t0, XMVectorMultiply(n, XMVector3Dot(n, t0))));
		XMVECTOR c = XMVector3Cross(n, t0);

		float dot = XMVectorGetX(XMVector3Dot(c, t1));
		float w = dot < 0 ? -1.0f : 0.0f;
		XMStoreFloat3(&vertices[i].Tangent, XMVectorScale(t, w));
	}

	delete[] tanA;
	delete[] tanB;
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
		offsetof(DemoVertex, Tangent),
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

	int diffuseWidth;
	int diffuseHeight;
	uint8_t* meshDiffuseTexture = stbi_load("assets/RoomThing_Albedo.png", &diffuseWidth, &diffuseHeight, nullptr, 4);
	int normalWidth;
	int normalHeight;
	uint8_t* meshNRMTexture = stbi_load("assets/RoomThing_NormalRoughnessMetallic.png", &normalWidth, &normalHeight, nullptr, 4);
	MaterialInfo meshMaterial = {
		{ &meshDiffuseTexture[0], BYTES_PER_PIXEL * diffuseWidth, diffuseWidth, diffuseHeight },
		{ &meshNRMTexture[0], BYTES_PER_PIXEL * normalWidth, normalWidth, normalHeight },
		{ 1.0f, 1.0f, 1.0f },
	};

	// Load the mesh in suzanne.obj
	tinyobj::ObjReaderConfig readerConfig;
	tinyobj::ObjReader reader;
	if (reader.ParseFromFile("assets/roomThing.obj"))
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
			uint32_t* indices = new uint32_t[triangleCount * 3];

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

			ComputeTangents(vertices, triangleCount * 3, indices, triangleCount * 3);

			MeshInfo meshInfo = { vertices, sizeof(vertices[0]), triangleCount * 3, indices, sizeof(indices[0]), triangleCount * 3};
			_Starlight->AddMesh(&meshInfo, &meshMaterial);

			delete[] indices;
			delete[] vertices;
		}
	}
	else
	{
		printf("[WARNING]: Could not load assets/suzanne.obj!");
	}

	stbi_image_free(meshDiffuseTexture);
	stbi_image_free(meshNRMTexture);

	//
	// DO BAKE
	//
	constexpr size_t outputStride = Align(BYTES_PER_PIXEL * LIGHT_MASK_RESOLUTION, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

#ifdef BAKE_POINT_LIGHT
	PointLight pointLight = { { 0, 1.1, 1 }, 0.1f, 1.0f, 0.09f, 0.032f };
	uint8_t* pointLightMaskBytes = new uint8_t[outputStride * LIGHT_MASK_RESOLUTION];
	OutputBuffer pointLightMask = { &pointLightMaskBytes[0], outputStride, LIGHT_MASK_RESOLUTION, LIGHT_MASK_RESOLUTION };
	_Starlight->BakePointLight(&pointLight, &pointLightMask);

	// Save point light's mask's pixel data, which is in `pointLightMaskBytes` - or do whatever else you want with it
	stbi_write_png("output.png", LIGHT_MASK_RESOLUTION, LIGHT_MASK_RESOLUTION, 4, pointLightMaskBytes, (int)outputStride);

	delete[] pointLightMaskBytes;
#endif

#ifdef BAKE_DIRECTIONAL_LIGHT
	DirectionalLight directionalLight = { { -1.0f, -3.0f, -2.0f } };
	uint8_t* directionalLightMaskBytes = new uint8_t[outputStride * LIGHT_MASK_RESOLUTION];
	OutputBuffer directionalLightMask = { &directionalLightMaskBytes[0], outputStride, LIGHT_MASK_RESOLUTION, LIGHT_MASK_RESOLUTION };
	_Starlight->BakeDirectionalLight(&directionalLight, &directionalLightMask);

	stbi_write_png("output_directional.png", LIGHT_MASK_RESOLUTION, LIGHT_MASK_RESOLUTION, 4, directionalLightMaskBytes, (int)outputStride);

	delete[] directionalLightMaskBytes;
#endif

#ifdef BAKE_SKY_LIGHT
	// Load skybox
	const char* skyboxFaceFilenames[] = {
		"assets/skybox/px.hdr",
		"assets/skybox/nx.hdr",
		"assets/skybox/py.hdr",
		"assets/skybox/ny.hdr",
		"assets/skybox/pz.hdr",
		"assets/skybox/nz.hdr",
	};
	SkyLight skybox = {
		{
			{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr },
			0, 0,
		}
	};
	for (size_t i = 0; i < SKYBOX_FACE_COUNT; i++)
	{
		int width = 0;
		int height = 0;
		int components = 0;
		skybox.Skybox.FacePixelData[i] = stbi_loadf(skyboxFaceFilenames[i], &width, &height, &components, 4);
		skybox.Skybox.Resolution = height;
		skybox.Skybox.Stride = width * sizeof(float) * 4;
	}
	uint8_t* skyLightMaskBytes = new uint8_t[outputStride * LIGHT_MASK_RESOLUTION];
	OutputBuffer skyLightMask = { &skyLightMaskBytes[0], outputStride, LIGHT_MASK_RESOLUTION, LIGHT_MASK_RESOLUTION };
	_Starlight->BakeSkyLight(&skybox, &skyLightMask);
	stbi_write_png("output_sky.png", LIGHT_MASK_RESOLUTION, LIGHT_MASK_RESOLUTION, 4, skyLightMaskBytes, (int)outputStride);
	for (size_t i = 0; i < SKYBOX_FACE_COUNT; i++)
	{
		stbi_image_free((void*)skybox.Skybox.FacePixelData[i]);
		skybox.Skybox.FacePixelData[i] = nullptr;
	}
#endif

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