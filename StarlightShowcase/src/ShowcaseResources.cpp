#include "ShowcaseResources.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d11.h>

#include "stb_image.h"

#include "res/ic_tool_move.h"
#include "res/ic_tool_rotate.h"
#include "res/ic_tool_select.h"

ID3D11ShaderResourceView* ShowcaseResources::LoadTexture(ID3D11Device* device, const uint8_t* fileBytes, size_t fileLength)
{
	ID3D11Texture2D* texture = nullptr;
	int x;
	int y;
	int channels;
	uint8_t* pixels = stbi_load_from_memory(fileBytes, fileLength, &x, &y, &channels, 4);

	D3D11_TEXTURE2D_DESC description =
	{
		(UINT)x,
		(UINT)y,
		1,
		1,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		{ 1, 0 },
		D3D11_USAGE_IMMUTABLE,
		D3D11_BIND_SHADER_RESOURCE,
		0,
		0,
	};
	D3D11_SUBRESOURCE_DATA initialData =
	{
		pixels,
		4 * x, // 4 bytes per pixel, no row padding
		0,
	};
	HRESULT hr = device->CreateTexture2D(&description, &initialData, &texture);
	stbi_image_free(pixels);
	if (FAILED(hr))
	{
		OutputDebugStringA("Failed to load Showcase texture!\n");
		return nullptr;
	}

	ID3D11ShaderResourceView* srv = nullptr;
	hr = device->CreateShaderResourceView(texture, nullptr, &srv);
	texture->Release();
	if (FAILED(hr))
	{
		OutputDebugStringA("Failed to create SRV for Showcase texture!\n");
		return nullptr;
	}

	return srv;
}

ShowcaseResources::ShowcaseResources(ID3D11Device* device)
{
	this->_IconSRVs[ShowcaseIcon_ToolMove] = this->LoadTexture(device, Resources::ic_tool_moveBytes, Resources::ic_tool_moveLength);
	this->_IconSRVs[ShowcaseIcon_ToolRotate] = this->LoadTexture(device, Resources::ic_tool_rotateBytes, Resources::ic_tool_rotateLength);
	this->_IconSRVs[ShowcaseIcon_ToolSelect] = this->LoadTexture(device, Resources::ic_tool_selectBytes, Resources::ic_tool_selectLength);
}

ShowcaseResources::~ShowcaseResources()
{
	for (int i = 0; i < ShowcaseIcon_COUNT; i++)
	{
		this->_IconSRVs[i]->Release();
		this->_IconSRVs[i] = nullptr;
	}
}